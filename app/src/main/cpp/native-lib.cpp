
#include <jni.h>
#include <cstdio>
#include <cstdarg>
#include <string>
#include <vector>
#include <mutex>
#include <android/log.h>
#include "native-lib.h"

extern "C" {
#include "libavformat/avformat.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavutil/error.h>
#include <libavutil/avutil.h>
#include <libavutil/timestamp.h>
//#include <ffmpeg.h>
}


extern "C"
JNIEXPORT jstring
Java_com_example_videoapp_utils_FFmpeg_ffmpegVersion(JNIEnv
                                                     *env, jobject /*thiz*/) {
    unsigned ver = avformat_version();
    char buf[64];
    std::snprintf(buf,
                  sizeof(buf), "avformat_version=%u", ver);
    return env->
            NewStringUTF(buf);
}

extern "C"
JNIEXPORT jstring
JNICALL
Java_com_example_videoapp_utils_FFmpeg_ffmpegConfig(JNIEnv *env, jobject
thiz) {
    return env->NewStringUTF(avcodec_configuration());
}



extern "C" JNIEXPORT jboolean

JNICALL
Java_com_example_videoapp_utils_FFmpeg_hasGifEncoder(JNIEnv *env, jobject /*thiz*/) {
    const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_GIF);
    return codec ? JNI_TRUE : JNI_FALSE;
}

extern "C"
JNIEXPORT jstring
JNICALL
Java_com_example_videoapp_utils_FFmpeg_getLastError(JNIEnv *env, jobject thiz) {
    return env->NewStringUTF(ff_get_last_error());
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_videoapp_utils_FFmpeg_initLogger(JNIEnv *env, jobject thiz
) {
    ff_init_logger();
}


static void android_log_callback(void *ptr, int level, const char *fmt, va_list vl) {
    if (level > av_log_get_level()) return;
    char line[1024];
    vsnprintf(line, sizeof(line), fmt, vl);

#ifdef __ANDROID__
    int prio = ANDROID_LOG_DEBUG;
    switch (level) {
        case AV_LOG_PANIC:
        case AV_LOG_FATAL:
            prio = ANDROID_LOG_FATAL;
            break;
        case AV_LOG_ERROR:
            prio = ANDROID_LOG_ERROR;
            break;
        case AV_LOG_WARNING:
            prio = ANDROID_LOG_WARN;
            break;
        case AV_LOG_INFO:
            prio = ANDROID_LOG_INFO;
            break;
        default:
            prio = ANDROID_LOG_DEBUG;
            break;
    }
    __android_log_print(prio, FF_LOG_TAG, "%s", line);
#else
    fprintf(stderr, "[%d] %s\n", level, line);
#endif
}

void ff_init_logger(int level) {
    av_log_set_level(level);
    av_log_set_callback(android_log_callback);
}

static thread_local char t_lastError[512];
static std::mutex g_errMutex;

void ff_store_last_error(int err, const char *where) {
    char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
    av_strerror(err, errbuf, sizeof(errbuf));
    {
        std::lock_guard<std::mutex> lk(g_errMutex);
        snprintf(t_lastError, sizeof(t_lastError), "%s failed: (%d) %s", where, err, errbuf);
    }
#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_ERROR, FF_LOG_TAG, "ERROR %s", t_lastError);
#endif
}

const char *ff_get_last_error() {
    std::lock_guard<std::mutex> lk(g_errMutex);
    return t_lastError[0] ? t_lastError : "";
}

struct GifResources {
    AVFormatContext *inFmtCtx = nullptr;
    AVFormatContext *outFmtCtx = nullptr;
    AVCodecContext *decCtx = nullptr;
    AVCodecContext *gifEncCtx = nullptr;

    AVStream *inVideoStream = nullptr;
    AVStream *outVideoStream = nullptr;

    AVPacket *pkt = nullptr;
    AVFrame *decFrame = nullptr;
    AVFrame *filtFrame = nullptr;

    AVFilterGraph *filterGraph = nullptr;
    AVFilterContext *buffersrcCtx = nullptr;
    AVFilterContext *buffersinkCtx = nullptr;

    ~GifResources() {
        if (pkt) av_packet_free(&pkt);
        if (decFrame) av_frame_free(&decFrame);
        if (filtFrame) av_frame_free(&filtFrame);
        if (decCtx) avcodec_free_context(&decCtx);
        if (gifEncCtx) avcodec_free_context(&gifEncCtx);
        if (inFmtCtx) avformat_close_input(&inFmtCtx);
        if (outFmtCtx) {
            if (!(outFmtCtx->oformat->flags & AVFMT_NOFILE) && outFmtCtx->pb) {
                avio_closep(&outFmtCtx->pb);
            }
            avformat_free_context(outFmtCtx);
        }
        if (filterGraph) avfilter_graph_free(&filterGraph);
    }
};


struct JStr {
    JNIEnv *env;
    jstring js;
    const char *cstr;

    JStr(JNIEnv *e, jstring s) : env(e), js(s) {
        cstr = s ? e->GetStringUTFChars(s, nullptr) : nullptr;
    }

    ~JStr() {
        if (cstr) env->ReleaseStringUTFChars(js, cstr);
    }
};

struct StageInfo {
    const char *name;
    double weight;
};

static const StageInfo STAGES[] = {
        {"open_input",         0.05},
        {"find_stream_info",   0.05},
        {"alloc_copy_streams", 0.08},
        {"open_output_io",     0.02},
        {"seek",               0.02},
        {"write_header",       0.03},
        {"process_packets",    0.70},
        {"write_trailer",      0.05}
};
#undef FAIL
#undef CHECK
#define FAIL(code, where) do { ret = (code); ff_store_last_error(ret, where); return ret; } while(0)
#define CHECK(EXPR, where) do { if ((ret = (EXPR)) < 0) { ff_store_last_error(ret, where); return ret; } } while(0)

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_videoapp_utils_FFmpeg_videoToGif(
        JNIEnv *env,
        jobject thiz,
        jstring jInput,
        jstring jOutput,
        jint jMaxWidth,
        jint jFps,
        jlong jStartMs,
        jlong jDurationMs,
        jint jQuality,
        jobject jListener) {

    JStr inPath(env, jInput), outPath(env, jOutput);
    GifResources R;

    int fps = jFps > 0 ? jFps : 10;
    int maxWidth = jMaxWidth > 0 ? jMaxWidth : 480;
    int64_t startMs = jStartMs;
    int64_t durationMs = jDurationMs;
    int64_t startPts = AV_NOPTS_VALUE;
    int64_t endPts = AV_NOPTS_VALUE;

    av_log(nullptr, AV_LOG_INFO, "videoToGif in=%s out=%s fps=%d maxW=%d",
           inPath.cstr, outPath.cstr, fps, maxWidth);

    auto run = [&]() -> int {
        int ret = 0;
        int64_t stat_readPkts = 0;
        int64_t stat_videoPkts = 0;
        int64_t stat_decFrames = 0;
        int64_t stat_filtFrames = 0;
        int64_t stat_encPkts = 0;
        int64_t stat_writtenPkts = 0;
        // 打开输入
        CHECK(avformat_open_input(&R.inFmtCtx, inPath.cstr, nullptr, nullptr), "open_input");
        CHECK(avformat_find_stream_info(R.inFmtCtx, nullptr), "find_stream_info");
        // 查找视频流
        {
            int vIndex = -1;
            for (unsigned i = 0; i < R.inFmtCtx->nb_streams; ++i) {
                if (R.inFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                    vIndex = (int) i;
                    break;
                }
            }
            if (vIndex < 0) FAIL(AVERROR_STREAM_NOT_FOUND, "video_stream");
            R.inVideoStream = R.inFmtCtx->streams[vIndex];
            av_log(nullptr, AV_LOG_INFO, "videoToGif: video_stream=%d tb=%d/%d", vIndex,
                   R.inVideoStream->time_base.num, R.inVideoStream->time_base.den);
        }

        // 创建解码器上下文
        {
            const AVCodec *dec = avcodec_find_decoder(R.inVideoStream->codecpar->codec_id);
            if (!dec) FAIL(AVERROR_DECODER_NOT_FOUND, "find_decoder");
            R.decCtx = avcodec_alloc_context3(dec);
            if (!R.decCtx) FAIL(AVERROR(ENOMEM), "alloc_dec_ctx");
            CHECK(avcodec_parameters_to_context(R.decCtx, R.inVideoStream->codecpar), "par_to_ctx");
            R.decCtx->thread_count = 2;
            CHECK(avcodec_open2(R.decCtx, dec, nullptr), "open_decoder");
        }

        AVRational inTb = R.inVideoStream->time_base;
        if (startMs > 0) {
            startPts = av_rescale_q(startMs, AVRational{1, 1000}, inTb);
        }
        if (durationMs > 0) {
            endPts = av_rescale_q(startMs + durationMs, AVRational{1, 1000}, inTb);
        }
        av_log(nullptr, AV_LOG_INFO,
               "videoToGif: cut range startMs=%lld durationMs=%lld startPts=%lld endPts=%lld tb=%d/%d",
               (long long) startMs, (long long) durationMs, (long long) startPts,
               (long long) endPts, inTb.num, inTb.den);

        if (startPts != AV_NOPTS_VALUE) {
            int sret = av_seek_frame(R.inFmtCtx, R.inVideoStream->index, startPts,
                                     AVSEEK_FLAG_BACKWARD);
            av_log(nullptr, AV_LOG_INFO, "videoToGif: seek to %lld ret=%d", (long long) startPts,
                   sret);
            if (sret >= 0) avcodec_flush_buffers(R.decCtx);
        }

        // 输出上下文
        {
            CHECK(avformat_alloc_output_context2(&R.outFmtCtx, nullptr, "gif", outPath.cstr),
                  "alloc_output_context2");
            if (!R.outFmtCtx) FAIL(AVERROR_UNKNOWN, "alloc_output_ctx_null");
        }

        // GIF 编码器
        const AVCodec *gifCodec = avcodec_find_encoder(AV_CODEC_ID_GIF);
        if (!gifCodec) FAIL(AVERROR_ENCODER_NOT_FOUND, "find_gif_encoder");

        R.gifEncCtx = avcodec_alloc_context3(gifCodec);
        if (!R.gifEncCtx) FAIL(AVERROR(ENOMEM), "alloc_gif_ctx");

        if (R.decCtx->width <= 0 || R.decCtx->height <= 0)
            FAIL(AVERROR_INVALIDDATA, "invalid_input_size");

        double aspect = (double) R.decCtx->height / (double) R.decCtx->width;
        int outW = maxWidth;
        int outH = (int) (outW * aspect);
        if (outH % 2) outH++;
        av_log(nullptr, AV_LOG_INFO, "videoToGif: enc size %dx%d fps=%d", outW, outH, fps);

        R.gifEncCtx->pix_fmt = AV_PIX_FMT_PAL8;
        R.gifEncCtx->time_base = (AVRational) {1, fps};
        R.gifEncCtx->width = outW;
        R.gifEncCtx->height = outH;

        R.outVideoStream = avformat_new_stream(R.outFmtCtx, gifCodec);
        if (!R.outVideoStream) FAIL(AVERROR(ENOMEM), "new_stream");

        CHECK(avcodec_open2(R.gifEncCtx, gifCodec, nullptr), "open_gif_encoder");
        CHECK(avcodec_parameters_from_context(R.outVideoStream->codecpar, R.gifEncCtx),
              "ctx_to_par");
        R.outVideoStream->time_base = R.gifEncCtx->time_base;

        if (!(R.outFmtCtx->oformat->flags & AVFMT_NOFILE)) {
            CHECK(avio_open(&R.outFmtCtx->pb, outPath.cstr, AVIO_FLAG_WRITE), "avio_open");
        }
        CHECK(avformat_write_header(R.outFmtCtx, nullptr), "write_header");

        // 分配帧/包
        R.pkt = av_packet_alloc();
        R.decFrame = av_frame_alloc();
        R.filtFrame = av_frame_alloc();
        if (!R.pkt || !R.decFrame || !R.filtFrame)
            FAIL(AVERROR(ENOMEM), "alloc_frames");

        // 过滤器图
        R.filterGraph = avfilter_graph_alloc();
        if (!R.filterGraph) FAIL(AVERROR(ENOMEM), "alloc_graph");

        {
            const AVFilter *buffersrc = avfilter_get_by_name("buffer");
            const AVFilter *buffersink = avfilter_get_by_name("buffersink");
            if (!buffersrc || !buffersink) FAIL(AVERROR_FILTER_NOT_FOUND, "find_filters");

            char args[256];
            snprintf(args, sizeof(args),
                     "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
                     R.decCtx->width, R.decCtx->height, R.decCtx->pix_fmt,
                     inTb.num, inTb.den,
                     R.decCtx->sample_aspect_ratio.num, R.decCtx->sample_aspect_ratio.den);

            CHECK(avfilter_graph_create_filter(&R.buffersrcCtx, buffersrc, "in", args, nullptr,
                                               R.filterGraph),
                  "create_buffersrc");
            CHECK(avfilter_graph_create_filter(&R.buffersinkCtx, buffersink, "out", nullptr,
                                               nullptr,
                                               R.filterGraph),
                  "create_buffersink");

            enum AVPixelFormat pixFmts[] = {AV_PIX_FMT_PAL8, AV_PIX_FMT_NONE};
            CHECK(av_opt_set_int_list(R.buffersinkCtx, "pix_fmts", pixFmts, AV_PIX_FMT_NONE,
                                      AV_OPT_SEARCH_CHILDREN), "set_pix_fmts");

            // 滤镜链：抽帧→缩放→调色板→应用调色板
            char filterDesc[512];
            snprintf(filterDesc, sizeof(filterDesc),
                     "[in]fps=%d,scale=%d:-1:flags=lanczos,split[a][b];"
                     "[a]palettegen=stats_mode=full[p];"
                     "[b][p]paletteuse=dither=bayer:bayer_scale=5[out]",
                     fps, maxWidth);
            av_log(nullptr, AV_LOG_INFO, "FilterGraph: %s", filterDesc);

            AVFilterInOut *inputs = avfilter_inout_alloc();
            AVFilterInOut *outputs = avfilter_inout_alloc();
            if (!inputs || !outputs) {
                if (inputs) avfilter_inout_free(&inputs);
                if (outputs) avfilter_inout_free(&outputs);
                FAIL(AVERROR(ENOMEM), "alloc_inout");
            }

            outputs->name = av_strdup("in");
            outputs->filter_ctx = R.buffersrcCtx;
            outputs->pad_idx = 0;
            outputs->next = nullptr;

            inputs->name = av_strdup("out");
            inputs->filter_ctx = R.buffersinkCtx;
            inputs->pad_idx = 0;
            inputs->next = nullptr;

            int pret = avfilter_graph_parse_ptr(R.filterGraph, filterDesc, &inputs, &outputs,
                                                nullptr);
            avfilter_inout_free(&inputs);
            avfilter_inout_free(&outputs);
            if (pret < 0) FAIL(pret, "graph_parse");
            CHECK(avfilter_graph_config(R.filterGraph, nullptr), "graph_config");
        }

        // 主处理循环
        {
            int ret2 = 0;
            int64_t gifFrameCount = 0;
            while ((ret2 = av_read_frame(R.inFmtCtx, R.pkt)) >= 0) {
                stat_readPkts++;
                if (R.pkt->stream_index != R.inVideoStream->index) {
                    av_packet_unref(R.pkt);
                    continue;
                }
                stat_videoPkts++;
                if (startPts != AV_NOPTS_VALUE && R.pkt->pts != AV_NOPTS_VALUE &&
                    R.pkt->pts < startPts) {
                    av_packet_unref(R.pkt);
                    continue;
                }
                if (endPts != AV_NOPTS_VALUE && R.pkt->pts != AV_NOPTS_VALUE &&
                    R.pkt->pts > endPts) {
                    av_packet_unref(R.pkt);
                    break;
                }

                ret2 = avcodec_send_packet(R.decCtx, R.pkt);
                av_packet_unref(R.pkt);
                if (ret2 < 0 && ret2 != AVERROR(EAGAIN)) FAIL(ret2, "send_packet");

                while ((ret2 = avcodec_receive_frame(R.decCtx, R.decFrame)) >= 0) {
                    stat_decFrames++;
                    R.decFrame->pts = R.decFrame->best_effort_timestamp;
                    CHECK(av_buffersrc_add_frame_flags(R.buffersrcCtx, R.decFrame,
                                                       AV_BUFFERSRC_FLAG_KEEP_REF),
                          "buffersrc_add_frame");
                    av_frame_unref(R.decFrame);

                    while ((ret2 = av_buffersink_get_frame(R.buffersinkCtx, R.filtFrame)) >= 0) {
                        stat_filtFrames++;
                        gifFrameCount++;
                        R.filtFrame->pts = gifFrameCount - 1;
                        ret2 = avcodec_send_frame(R.gifEncCtx, R.filtFrame);
                        av_frame_unref(R.filtFrame);
                        if (ret2 < 0 && ret2 != AVERROR(EAGAIN)) FAIL(ret2, "gif_send_frame");

                        while ((ret2 = avcodec_receive_packet(R.gifEncCtx, R.pkt)) >= 0) {
                            stat_encPkts++;
                            av_packet_rescale_ts(R.pkt, R.gifEncCtx->time_base,
                                                 R.outVideoStream->time_base);
                            R.pkt->stream_index = R.outVideoStream->index;
                            int wret = av_interleaved_write_frame(R.outFmtCtx, R.pkt);
                            if (wret < 0) {
                                ff_store_last_error(wret, "write_frame");
                                av_packet_unref(R.pkt);
                                return wret;
                            }
                            stat_writtenPkts++;
                            av_packet_unref(R.pkt);
                        }
                        if (ret2 == AVERROR(EAGAIN) || ret2 == AVERROR_EOF) ret2 = 0;
                    }
                    if (ret2 == AVERROR(EAGAIN) || ret2 == AVERROR_EOF) ret2 = 0;
                }
                if (ret2 == AVERROR(EAGAIN) || ret2 == AVERROR_EOF) ret2 = 0;
            }
            if (ret2 != AVERROR_EOF && ret2 != 0) FAIL(ret2, "read_frame_loop");

            // flush 解码器
            ret2 = avcodec_send_packet(R.decCtx, nullptr);
            if (ret2 >= 0) {
                while ((ret2 = avcodec_receive_frame(R.decCtx, R.decFrame)) >= 0) {
                    stat_decFrames++;
                    R.decFrame->pts = R.decFrame->best_effort_timestamp;
                    if (av_buffersrc_add_frame_flags(R.buffersrcCtx, R.decFrame,
                                                     AV_BUFFERSRC_FLAG_KEEP_REF) < 0)
                        break;
                    av_frame_unref(R.decFrame);

                    while (av_buffersink_get_frame(R.buffersinkCtx, R.filtFrame) >= 0) {
                        stat_filtFrames++;
                        gifFrameCount++;
                        R.filtFrame->pts = gifFrameCount - 1;
                        if (avcodec_send_frame(R.gifEncCtx, R.filtFrame) >= 0) {
                            while (avcodec_receive_packet(R.gifEncCtx, R.pkt) >= 0) {
                                stat_encPkts++;
                                av_packet_rescale_ts(R.pkt, R.gifEncCtx->time_base,
                                                     R.outVideoStream->time_base);
                                R.pkt->stream_index = R.outVideoStream->index;
                                if (av_interleaved_write_frame(R.outFmtCtx, R.pkt) >= 0) {
                                    stat_writtenPkts++;
                                }
                                av_packet_unref(R.pkt);
                            }
                        }
                        av_frame_unref(R.filtFrame);
                    }
                }
            }

            // flush 过滤器（通知 EOF 并把剩余帧取完）
            av_buffersrc_add_frame_flags(R.buffersrcCtx, nullptr, 0);
            while (av_buffersink_get_frame(R.buffersinkCtx, R.filtFrame) >= 0) {
                stat_filtFrames++;
                gifFrameCount++;
                R.filtFrame->pts = gifFrameCount - 1;
                if (avcodec_send_frame(R.gifEncCtx, R.filtFrame) >= 0) {
                    while (avcodec_receive_packet(R.gifEncCtx, R.pkt) >= 0) {
                        stat_encPkts++;
                        av_packet_rescale_ts(R.pkt, R.gifEncCtx->time_base,
                                             R.outVideoStream->time_base);
                        R.pkt->stream_index = R.outVideoStream->index;
                        if (av_interleaved_write_frame(R.outFmtCtx, R.pkt) >= 0) {
                            stat_writtenPkts++;
                        }
                        av_packet_unref(R.pkt);
                    }
                }
                av_frame_unref(R.filtFrame);
            }

            // flush GIF 编码器
            avcodec_send_frame(R.gifEncCtx, nullptr);
            while (avcodec_receive_packet(R.gifEncCtx, R.pkt) >= 0) {
                stat_encPkts++;
                av_packet_rescale_ts(R.pkt, R.gifEncCtx->time_base, R.outVideoStream->time_base);
                R.pkt->stream_index = R.outVideoStream->index;
                if (av_interleaved_write_frame(R.outFmtCtx, R.pkt) >= 0) {
                    stat_writtenPkts++;
                }
                av_packet_unref(R.pkt);
            }

            av_log(nullptr, AV_LOG_INFO,
                   "videoToGif stats: readPkts=%lld videoPkts=%lld decFrames=%lld filtFrames=%lld encPkts=%lld writtenPkts=%lld",
                   (long long) stat_readPkts, (long long) stat_videoPkts,
                   (long long) stat_decFrames,
                   (long long) stat_filtFrames, (long long) stat_encPkts,
                   (long long) stat_writtenPkts);

            CHECK(av_write_trailer(R.outFmtCtx), "write_trailer");
            av_log(nullptr, AV_LOG_INFO, "GIF success");
        }

        return 0;
    };

    int ret = run();
    if (ret < 0) {
        av_log(nullptr, AV_LOG_ERROR, "videoToGif failed ret=%d (%s)", ret,

               ff_get_last_error()

        );
    }

    return
            ret;
}


extern "C"
JNIEXPORT jint JNICALL
Java_com_example_videoapp_utils_FFmpeg_clip(
        JNIEnv *env,
        jobject /*thiz*/,
        jstring jInput,
        jstring jOutput,
        jlong
        jStartMs,
        jlong jDurationMs,
        jboolean
        jReEncode,
        jobject jListener
) {
    int ret = 0;
#undef FAIL
#undef CHECK
#define FAIL(code, where) do { ret = (code); ff_store_last_error(ret, where); return ret; } while(0)
#define CHECK(EXPR, where) do { if ((ret = (EXPR)) < 0) { ff_store_last_error(ret, where); return ret; } } while(0)

    JStr inPath(env, jInput), outPath(env, jOutput);
    const bool reEncode = (jReEncode == JNI_TRUE);

    int64_t startMs = (int64_t) jStartMs;
    int64_t durationMs = (int64_t) jDurationMs;

//    jclass listenerCls = nullptr;
//    jmethodID midOnProgress = nullptr;
//    jmethodID midIsCancelled = nullptr;
//    jmethodID midOnCompleted = nullptr;
//    jmethodID midOnError = nullptr;
//    if (jListener) {
//        listenerCls = env->GetObjectClass(jListener);
//        midOnProgress = env->GetMethodID(listenerCls, "onProgress", "(FILjava/lang/String;)V");
//        if (env->
//
//                ExceptionCheck()
//
//                )
//            env->
//
//                    ExceptionClear();
//
//        midIsCancelled = env->GetMethodID(listenerCls, "isCancelled", "()Z");
//        if (env->
//
//                ExceptionCheck()
//
//                )
//            env->
//
//                    ExceptionClear();
//
//        midOnCompleted = env->GetMethodID(listenerCls, "onCompleted", "(I)V");
//        if (env->
//
//                ExceptionCheck()
//
//                )
//            env->
//
//                    ExceptionClear();
//
//        midOnError = env->GetMethodID(listenerCls, "onError", "(ILjava/lang/String;)V");
//        if (env->
//
//                ExceptionCheck()
//
//                )
//            env->
//
//                    ExceptionClear();
//
//    }
//    static const int STAGE_COUNT = (int) (sizeof(STAGES) / sizeof(STAGES[0]));
//
//    double prefix[STAGE_COUNT + 1];
//    prefix[0] = 0.0;
//    for (
//            int i = 0;
//            i < STAGE_COUNT;
//            i++) {
//        prefix[i + 1] = prefix[i] + STAGES[i].
//                weight;
//    }
//
//    auto callProgress = [&](int stageIndex, double intra) {
//        if (!jListener || !midOnProgress) return;
//        if (stageIndex < 0) stageIndex = 0;
//        if (stageIndex >= STAGE_COUNT) stageIndex = STAGE_COUNT - 1;
//        if (intra < 0.0) intra = 0.0;
//        if (intra > 1.0) intra = 1.0;
//        double base = prefix[stageIndex];
//        double w = STAGES[stageIndex].weight;
//        double prog = base + w * intra;
//        if (prog > 0.999999 && stageIndex == STAGE_COUNT - 1 && intra >= 1.0) {
//            prog = 0.999999; // 留一点给最终 DONE=1.0
//        }
//        jstring jName = env->NewStringUTF(STAGES[stageIndex].name);
//        env->CallVoidMethod(jListener, midOnProgress, (jfloat) prog, (jint) stageIndex, jName);
//        if (env->ExceptionCheck()) env->ExceptionClear();
//        env->DeleteLocalRef(jName);
//    };
//
//    auto callFinal = [&](int code, bool success) {
//        if (jListener) {
//            if (success && midOnProgress) {
//                jstring jName = env->NewStringUTF("done");
//                env->CallVoidMethod(jListener, midOnProgress, (jfloat) 1.0f,
//                                    (jint) STAGE_COUNT, jName);
//                if (env->ExceptionCheck()) env->ExceptionClear();
//                env->DeleteLocalRef(jName);
//            }
//            if (code == 0 && midOnCompleted) {
//                env->CallVoidMethod(jListener, midOnCompleted, (jint) 0);
//                if (env->ExceptionCheck()) env->ExceptionClear();
//            } else if (code != 0 && midOnError) {
//                jstring msg = env->NewStringUTF(ff_get_last_error());
//                env->CallVoidMethod(jListener, midOnError, (jint) code, msg);
//                if (env->ExceptionCheck()) env->ExceptionClear();
//                env->DeleteLocalRef(msg);
//            }
//        }
//    };
//
//    auto isCancelled = [&]() -> bool {
//        if (!jListener || !midIsCancelled) return false;
//        jboolean c = env->CallBooleanMethod(jListener, midIsCancelled);
//        if (env->ExceptionCheck()) {
//            env->ExceptionClear();
//            return false;
//        }
//        return c == JNI_TRUE;
//    };

// 参数校验
    if (startMs < 0) FAIL(AVERROR(EINVAL), "clip:startMs<0");
    if (durationMs < 0) FAIL(AVERROR(EINVAL), "clip:durationMs<0");
    if (durationMs == 0) {
        av_log(nullptr, AV_LOG_WARNING, "clip: durationMs==0 -> default 2000ms (2s)");
        durationMs = 2000; // 默认剪辑 2s
    }
    av_log(nullptr, AV_LOG_INFO,
           "clip begin in=%s out=%s startMs=%lld durationMs=%lld reEncode=%d",
           inPath.cstr, outPath.cstr, (long long) startMs, (long long) durationMs, (int) reEncode);

    auto run = [&]() -> int {
        struct ClipRes {
            AVFormatContext *inFmtCtx = nullptr;
            AVFormatContext *outFmtCtx = nullptr;

            AVPacket *pkt = nullptr;

            ~ClipRes() {
                if (pkt) av_packet_free(&pkt);
                if (outFmtCtx) {
                    if (!(outFmtCtx->oformat->flags & AVFMT_NOFILE) && outFmtCtx->pb) {
                        avio_closep(&outFmtCtx->pb);
                    }
                    avformat_free_context(outFmtCtx);
                }
                if (inFmtCtx) avformat_close_input(&inFmtCtx);
            }
        } R;

        // 打开输入并读取信息
        CHECK(avformat_open_input(&R.inFmtCtx, inPath.cstr, nullptr, nullptr), "clip:open_input");
        CHECK(avformat_find_stream_info(R.inFmtCtx, nullptr), "clip:find_stream_info");
        av_log(nullptr, AV_LOG_INFO, "clip: input nb_streams=%u duration(us)=%lld",
               R.inFmtCtx->nb_streams, (long long) R.inFmtCtx->duration);
        av_dump_format(R.inFmtCtx, 0, inPath.cstr, 0);

        int64_t totalDurationMs = (R.inFmtCtx->duration > 0)
                                  ? R.inFmtCtx->duration / (AV_TIME_BASE / 1000)
                                  : -1;
        if (totalDurationMs > 0 && startMs >= totalDurationMs)
            FAIL(AVERROR(EINVAL), "clip:start>=total");
        if (totalDurationMs > 0 && startMs + durationMs > totalDurationMs) {
            av_log(nullptr, AV_LOG_WARNING,
                   "clip: endMs(%lld) > total(%lld), truncating",
                   (long long) (startMs + durationMs), (long long) totalDurationMs);
            durationMs = totalDurationMs - startMs;
        }

        if (!reEncode) {
            // 输出上下文
            CHECK(avformat_alloc_output_context2(&R.outFmtCtx, nullptr, nullptr, outPath.cstr),
                  "clip:alloc_output_ctx");

            // 为每个输入流创建对应输出流（copy 参数）
            int streamCount = (int) R.inFmtCtx->nb_streams;
            std::vector<int64_t> firstPts(streamCount, AV_NOPTS_VALUE);
            std::vector<int64_t> firstDts(streamCount, AV_NOPTS_VALUE);

            for (unsigned i = 0; i < R.inFmtCtx->nb_streams; ++i) {
                AVStream *inStream = R.inFmtCtx->streams[i];
                AVCodecParameters *inCodecPar = inStream->codecpar;
                AVStream *outStream = avformat_new_stream(R.outFmtCtx, nullptr);
                if (!outStream) FAIL(AVERROR(ENOMEM), "clip:new_stream");
                CHECK(avcodec_parameters_copy(outStream->codecpar, inCodecPar), "clip:copy_par");
                outStream->codecpar->codec_tag = 0; // 避免不兼容
                // 关键：复制 time_base，���免时间基差异导致时长计算偏差
                outStream->time_base = inStream->time_base;
            }

            // 打开输出 IO
            if (!(R.outFmtCtx->oformat->flags & AVFMT_NOFILE))
                CHECK(avio_open(&R.outFmtCtx->pb, outPath.cstr, AVIO_FLAG_WRITE), "clip:avio_open");

            // 选择用于 seek 的流（尽量视频）
            int videoStreamIndex = -1;
            for (unsigned i = 0; i < R.inFmtCtx->nb_streams; ++i) {
                if (R.inFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                    videoStreamIndex = (int) i;
                    break;
                }
            }
            int seekStream = (videoStreamIndex >= 0) ? videoStreamIndex : 0;
            AVRational tb = R.inFmtCtx->streams[seekStream]->time_base;
            av_log(nullptr, AV_LOG_INFO, "clip: seekStream=%d tb=%d/%d", seekStream, tb.num,
                   tb.den);

            int64_t endMs = startMs + durationMs;
            int64_t seekTarget = av_rescale_q(startMs, AVRational{1, 1000}, tb);
            int sret = av_seek_frame(R.inFmtCtx, seekStream, seekTarget, AVSEEK_FLAG_BACKWARD);
            av_log(nullptr, AV_LOG_INFO, "clip: av_seek_frame to %lld ret=%d",
                   (long long) seekTarget, sret);
            if (sret < 0) {
                av_log(nullptr, AV_LOG_WARNING, "clip: seek failed, fallback to start");
                avformat_seek_file(R.inFmtCtx, seekStream, INT64_MIN, 0, INT64_MAX, 0);
            }

            // 写文件头
            CHECK(avformat_write_header(R.outFmtCtx, nullptr), "clip:write_header");
            av_dump_format(R.outFmtCtx, 0, outPath.cstr, 1);

            // 主循环读取/写入
            R.pkt = av_packet_alloc();
            if (!R.pkt) FAIL(AVERROR(ENOMEM), "clip:alloc_pkt");

            int64_t stat_read = 0, stat_written = 0;
            while ((ret = av_read_frame(R.inFmtCtx, R.pkt)) >= 0) {
                ++stat_read;
                AVStream *inStream = R.inFmtCtx->streams[R.pkt->stream_index];
                AVStream *outStream = R.outFmtCtx->streams[R.pkt->stream_index];
                AVRational inTb = inStream->time_base;
                int64_t curMs = (R.pkt->pts == AV_NOPTS_VALUE) ? -1
                                                               : av_rescale_q(R.pkt->pts, inTb,
                                                                              AVRational{1, 1000});

                if (curMs >= 0 && curMs < startMs) {
                    av_packet_unref(R.pkt);
                    continue;
                }
                if (curMs >= 0 && curMs > endMs) {
                    // 超过结束时间：主流则结束，其他流仅跳过，避���尾部卡帧
                    av_packet_unref(R.pkt);
                    if (R.pkt->stream_index == seekStream) break;
                    else continue;
                }

                if (firstPts[R.pkt->stream_index] == AV_NOPTS_VALUE && R.pkt->pts != AV_NOPTS_VALUE)
                    firstPts[R.pkt->stream_index] = R.pkt->pts;
                if (firstDts[R.pkt->stream_index] == AV_NOPTS_VALUE && R.pkt->dts != AV_NOPTS_VALUE)
                    firstDts[R.pkt->stream_index] = R.pkt->dts;

                if (R.pkt->pts != AV_NOPTS_VALUE && firstPts[R.pkt->stream_index] != AV_NOPTS_VALUE)
                    R.pkt->pts -= firstPts[R.pkt->stream_index];
                if (R.pkt->dts != AV_NOPTS_VALUE && firstDts[R.pkt->stream_index] != AV_NOPTS_VALUE)
                    R.pkt->dts -= firstDts[R.pkt->stream_index];

                R.pkt->pts = (R.pkt->pts == AV_NOPTS_VALUE) ? AV_NOPTS_VALUE : av_rescale_q(
                        R.pkt->pts, inTb, outStream->time_base);
                R.pkt->dts = (R.pkt->dts == AV_NOPTS_VALUE) ? AV_NOPTS_VALUE : av_rescale_q(
                        R.pkt->dts, inTb, outStream->time_base);
                R.pkt->duration = av_rescale_q(R.pkt->duration, inTb, outStream->time_base);
                R.pkt->pos = -1;
                // 移除不安全的 dts=pts 修正，避免 B 帧重排被破坏

                int wret = av_interleaved_write_frame(R.outFmtCtx, R.pkt);
                av_packet_unref(R.pkt);
                if (wret < 0) {
                    ff_store_last_error(wret, "clip:write_frame");
                    return wret;
                }
                ++stat_written;
            }

            if (ret == AVERROR_EOF) ret = 0;
            av_log(nullptr, AV_LOG_INFO, "clip: loop end ret=%d read=%lld written=%lld",
                   ret, (long long) stat_read, (long long) stat_written);
            if (ret >= 0) CHECK(av_write_trailer(R.outFmtCtx), "clip:write_trailer");
            av_log(nullptr, AV_LOG_INFO, "clip success written=%lld", (long long) stat_written);
            return ret;
        } else {
            FAIL(AVERROR(ENOSYS), "clip:reencode_not_impl");
        }
    };

    int rc = run();
    if (rc < 0) {
        av_log(nullptr, AV_LOG_ERROR, "clip failed rc=%d (%s)", rc,

               ff_get_last_error()

        );
    } else {
        av_log(nullptr, AV_LOG_INFO, "clip done rc=%d", rc);
    }
    return
            rc;
}
