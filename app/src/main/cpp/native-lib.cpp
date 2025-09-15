//// Write C++ code here.
////
//// Do not forget to dynamically load the C++ library into your application.
////
//// For instance,
////
//// In MainActivity.java:
////    static {
////       System.loadLibrary("videoapp");
////    }
////
//// Or, in MainActivity.kt:
////    companion object {
////      init {
////         System.loadLibrary("videoapp")
////      }
////    }
//
#include <jni.h>
#include <cstdio>
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
}
#define CHECK_STEP(expr, label, stepName) \
    do { \
        if ((ret = (expr)) < 0) { \
            ff_store_last_error(ret, stepName); \
            goto label; \
        } \
    } while(0)
extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_videoapp_utils_FFmpeg_ffmpegVersion(JNIEnv *env, jobject /*thiz*/) {
    unsigned ver = avformat_version();
    char buf[64];
    std::snprintf(buf, sizeof(buf), "avformat_version=%u", ver);
    return env->NewStringUTF(buf);
}
extern "C" {
#include "libavcodec/avcodec.h"
JNIEXPORT jstring JNICALL
Java_com_example_videoapp_utils_FFmpeg_ffmpegConfig(JNIEnv *env, jobject thiz) {
    return env->NewStringUTF(avcodec_configuration());}}



extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_videoapp_utils_FFmpeg_hasGifEncoder(JNIEnv *env, jobject /*thiz*/) {
    const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_GIF);
    return codec ? JNI_TRUE : JNI_FALSE;
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_videoapp_utils_FFmpeg_getLastError(JNIEnv *env, jobject thiz) {
    return env->NewStringUTF(ff_get_last_error());
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_videoapp_utils_FFmpeg_initLogger(JNIEnv *env, jobject thiz) {
    ff_init_logger();
}

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  "GIF", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "GIF", __VA_ARGS__)

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

static inline void logErr(int ret, const char *ctx) {
    char buf[256];
    av_strerror(ret, buf, sizeof(buf));
    LOGE("%s error: %s (%d)", ctx, buf, ret);
}

// RAII 包装 jstring -> const char*
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

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_videoapp_utils_FFmpeg_videoToGif(JNIEnv *env, jobject thiz,
                                                  jstring jInput,
                                                  jstring jOutput,
                                                  jint jMaxWidth,
                                                  jint jFps,
                                                  jlong jStartMs,
                                                  jlong jDurationMs,
                                                  jint /*qualityPreset*/) {
    JStr inPath(env, jInput);
    JStr outPath(env, jOutput);

    GifResources R;
    int ret = 0;

    int fps = jFps > 0 ? jFps : 10;
    int maxWidth = jMaxWidth > 0 ? jMaxWidth : 480;
    int64_t startMs = jStartMs;
    int64_t durationMs = jDurationMs;
    int64_t startPts = AV_NOPTS_VALUE;
    int64_t endPts = AV_NOPTS_VALUE;

    LOGI("videoToGif in=%s out=%s fps=%d maxW=%d", inPath.cstr, outPath.cstr, fps, maxWidth);

    // 输入
    ret = avformat_open_input(&R.inFmtCtx, inPath.cstr, nullptr, nullptr);

    if (ret < 0) {
        logErr(ret, "open_input");
        return ret;
    }
    ret = avformat_find_stream_info(R.inFmtCtx, nullptr);
    if (ret < 0) {
        logErr(ret, "find_stream_info");
        return ret;
    }

    // 找视频流
    {
        int idx = -1;
        for (unsigned i = 0; i < R.inFmtCtx->nb_streams; ++i) {
            if (R.inFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                idx = (int) i;
                break;
            }
        }
        if (idx < 0) {
            ret = AVERROR_STREAM_NOT_FOUND;
            logErr(ret, "video_stream");
            return ret;
        }
        R.inVideoStream = R.inFmtCtx->streams[idx];
    }

    // 解码器
    {
        const AVCodec *dec = avcodec_find_decoder(R.inVideoStream->codecpar->codec_id);
        if (!dec) {
            ret = AVERROR_DECODER_NOT_FOUND;
            logErr(ret, "find_decoder");
            return ret;
        }
        R.decCtx = avcodec_alloc_context3(dec);
        if (!R.decCtx) {
            ret = AVERROR(ENOMEM);
            logErr(ret, "alloc_dec_ctx");
            return ret;
        }
        ret = avcodec_parameters_to_context(R.decCtx, R.inVideoStream->codecpar);
        if (ret < 0) {
            logErr(ret, "par_to_ctx");
            return ret;
        }
        R.decCtx->thread_count = 2;
        ret = avcodec_open2(R.decCtx, dec, nullptr);
        if (ret < 0) {
            logErr(ret, "open_decoder");
            return ret;
        }
    }

    AVRational inTb = R.inVideoStream->time_base;
    if (startMs > 0) startPts = (int64_t) ((startMs / 1000.0) / av_q2d(inTb));
    if (durationMs > 0) endPts = (int64_t) (((startMs + durationMs) / 1000.0) / av_q2d(inTb));

    if (startPts != AV_NOPTS_VALUE) {
        int sret = av_seek_frame(R.inFmtCtx, R.inVideoStream->index, startPts,
                                 AVSEEK_FLAG_BACKWARD);
        if (sret >= 0) avcodec_flush_buffers(R.decCtx);
    }

    // 输出与编码器
    ret = avformat_alloc_output_context2(&R.outFmtCtx, nullptr, "gif", outPath.cstr);
    if (ret < 0 || !R.outFmtCtx) {
        logErr(ret, "alloc_output_context");
        return ret < 0 ? ret : AVERROR_UNKNOWN;
    }

    const AVCodec *gifCodec = avcodec_find_encoder(AV_CODEC_ID_GIF);
    if (!gifCodec) {
        ret = AVERROR_ENCODER_NOT_FOUND;
        logErr(ret, "find_gif_encoder");
        return ret;
    }
    R.gifEncCtx = avcodec_alloc_context3(gifCodec);
    if (!R.gifEncCtx) {
        ret = AVERROR(ENOMEM);
        logErr(ret, "alloc_gif_ctx");
        return ret;
    }

    if (R.decCtx->width <= 0 || R.decCtx->height <= 0) {
        ret = AVERROR_INVALIDDATA;
        logErr(ret, "invalid_input_size");
        return ret;
    }

    double ratio = R.decCtx->height * 1.0 / R.decCtx->width;
    int outW = maxWidth;
    int outH = (int) (outW * ratio);
    if (outH % 2) outH++;

    R.gifEncCtx->pix_fmt = AV_PIX_FMT_PAL8;
    R.gifEncCtx->time_base = (AVRational) {1, fps};
    R.gifEncCtx->width = outW;
    R.gifEncCtx->height = outH;

    R.outVideoStream = avformat_new_stream(R.outFmtCtx, gifCodec);
    if (!R.outVideoStream) {
        ret = AVERROR(ENOMEM);
        logErr(ret, "new_stream");
        return ret;
    }

    ret = avcodec_open2(R.gifEncCtx, gifCodec, nullptr);
    if (ret < 0) {
        logErr(ret, "open_gif_encoder");
        return ret;
    }

    ret = avcodec_parameters_from_context(R.outVideoStream->codecpar, R.gifEncCtx);
    if (ret < 0) {
        logErr(ret, "ctx_to_par");
        return ret;
    }
    R.outVideoStream->time_base = R.gifEncCtx->time_base;

    if (!(R.outFmtCtx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&R.outFmtCtx->pb, outPath.cstr, AVIO_FLAG_WRITE);
        if (ret < 0) {
            logErr(ret, "avio_open");
            return ret;
        }
    }
    ret = avformat_write_header(R.outFmtCtx, nullptr);
    if (ret < 0) {
        logErr(ret, "write_header");
        return ret;
    }

    // 帧/包
    R.pkt = av_packet_alloc();
    R.decFrame = av_frame_alloc();
    R.filtFrame = av_frame_alloc();
    if (!R.pkt || !R.decFrame || !R.filtFrame) {
        ret = AVERROR(ENOMEM);
        logErr(ret, "alloc_frames");
        return ret;
    }

    // 滤镜
    R.filterGraph = avfilter_graph_alloc();
    if (!R.filterGraph) {
        ret = AVERROR(ENOMEM);
        logErr(ret, "alloc_graph");
        return ret;
    }

    {
        const AVFilter *buffersrc = avfilter_get_by_name("buffer");
        const AVFilter *buffersink = avfilter_get_by_name("buffersink");
        if (!buffersrc || !buffersink) {
            ret = AVERROR_FILTER_NOT_FOUND;
            logErr(ret, "find_filters");
            return ret;
        }

        char args[256];
        snprintf(args, sizeof(args),
                 "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
                 R.decCtx->width, R.decCtx->height, R.decCtx->pix_fmt,
                 inTb.num, inTb.den,
                 R.decCtx->sample_aspect_ratio.num, R.decCtx->sample_aspect_ratio.den);

        ret = avfilter_graph_create_filter(&R.buffersrcCtx, buffersrc, "in", args, nullptr,
                                           R.filterGraph);
        if (ret < 0) {
            logErr(ret, "create_buffersrc");
            return ret;
        }
        ret = avfilter_graph_create_filter(&R.buffersinkCtx, buffersink, "out", nullptr, nullptr,
                                           R.filterGraph);
        if (ret < 0) {
            logErr(ret, "create_buffersink");
            return ret;
        }

        enum AVPixelFormat pixFmts[] = {AV_PIX_FMT_PAL8, AV_PIX_FMT_NONE};
        ret = av_opt_set_int_list(R.buffersinkCtx, "pix_fmts", pixFmts, AV_PIX_FMT_NONE,
                                  AV_OPT_SEARCH_CHILDREN);
        if (ret < 0) {
            logErr(ret, "set_pix_fmts");
            return ret;
        }

        char filterDesc[512];
        snprintf(filterDesc, sizeof(filterDesc),
                 "[0:v]fps=%d,scale=%d:-1:flags=lanczos,split[a][b];"
                 "[a]palettegen=stats_mode=full[p];"
                 "[b][p]paletteuse=dither=bayer:bayer_scale=5",
                 fps, maxWidth);
        LOGI("FilterGraph: %s", filterDesc);

        AVFilterInOut *inputs = avfilter_inout_alloc();
        AVFilterInOut *outputs = avfilter_inout_alloc();
        if (!inputs || !outputs) {
            if (inputs) avfilter_inout_free(&inputs);
            if (outputs) avfilter_inout_free(&outputs);
            ret = AVERROR(ENOMEM);
            logErr(ret, "alloc_inout");
            return ret;
        }
        outputs->name = av_strdup("in");
        outputs->filter_ctx = R.buffersrcCtx;
        outputs->pad_idx = 0;
        outputs->next = nullptr;

        inputs->name = av_strdup("out");
        inputs->filter_ctx = R.buffersinkCtx;
        inputs->pad_idx = 0;
        inputs->next = nullptr;

        ret = avfilter_graph_parse_ptr(R.filterGraph, filterDesc, &inputs, &outputs, nullptr);
        if (ret < 0) {
            logErr(ret, "graph_parse");
            avfilter_inout_free(&inputs);
            avfilter_inout_free(&outputs);
            return ret;
        }
        ret = avfilter_graph_config(R.filterGraph, nullptr);
        if (ret < 0) {
            logErr(ret, "graph_config");
            avfilter_inout_free(&inputs);
            avfilter_inout_free(&outputs);
            return ret;
        }
        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
    }

    // 主循环
    int64_t gifFrameCount = 0;
    while ((ret = av_read_frame(R.inFmtCtx, R.pkt)) >= 0) {
        if (R.pkt->stream_index != R.inVideoStream->index) {
            av_packet_unref(R.pkt);
            continue;
        }
        if (startPts != AV_NOPTS_VALUE && R.pkt->pts != AV_NOPTS_VALUE && R.pkt->pts < startPts) {
            av_packet_unref(R.pkt);
            continue;
        }
        if (endPts != AV_NOPTS_VALUE && R.pkt->pts != AV_NOPTS_VALUE && R.pkt->pts > endPts) {
            av_packet_unref(R.pkt);
            break;
        }

        ret = avcodec_send_packet(R.decCtx, R.pkt);
        av_packet_unref(R.pkt);
        if (ret < 0 && ret != AVERROR(EAGAIN)) {
            logErr(ret, "send_packet");
            return ret;
        }

        while ((ret = avcodec_receive_frame(R.decCtx, R.decFrame)) >= 0) {
            R.decFrame->pts = R.decFrame->best_effort_timestamp;
            ret = av_buffersrc_add_frame_flags(R.buffersrcCtx, R.decFrame,
                                               AV_BUFFERSRC_FLAG_KEEP_REF);
            av_frame_unref(R.decFrame);
            if (ret < 0) {
                logErr(ret, "buffersrc_add_frame");
                return ret;
            }

            // drain filter
            while ((ret = av_buffersink_get_frame(R.buffersinkCtx, R.filtFrame)) >= 0) {
                gifFrameCount++;
                R.filtFrame->pts = gifFrameCount - 1;

                ret = avcodec_send_frame(R.gifEncCtx, R.filtFrame);
                av_frame_unref(R.filtFrame);
                if (ret < 0 && ret != AVERROR(EAGAIN)) {
                    logErr(ret, "gif_send_frame");
                    return ret;
                }

                AVPacket outPkt;
                av_init_packet(&outPkt);
                while ((ret = avcodec_receive_packet(R.gifEncCtx, &outPkt)) >= 0) {
                    av_packet_rescale_ts(&outPkt, R.gifEncCtx->time_base,
                                         R.outVideoStream->time_base);
                    outPkt.stream_index = R.outVideoStream->index;
                    ret = av_interleaved_write_frame(R.outFmtCtx, &outPkt);
                    av_packet_unref(&outPkt);
                    if (ret < 0) {
                        logErr(ret, "write_frame");
                        return ret;
                    }
                }
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) ret = 0;
            }
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) ret = 0;
        }
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) ret = 0;
    }
    if (ret != AVERROR_EOF && ret != 0) {
        logErr(ret, "read_frame_loop");
        return ret;
    }

    // flush 解码器
    ret = avcodec_send_packet(R.decCtx, nullptr);
    if (ret >= 0) {
        while ((ret = avcodec_receive_frame(R.decCtx, R.decFrame)) >= 0) {
            R.decFrame->pts = R.decFrame->best_effort_timestamp;
            if (av_buffersrc_add_frame_flags(R.buffersrcCtx, R.decFrame,
                                             AV_BUFFERSRC_FLAG_KEEP_REF) < 0)
                break;
            av_frame_unref(R.decFrame);
            while (av_buffersink_get_frame(R.buffersinkCtx, R.filtFrame) >= 0) {
                gifFrameCount++;
                R.filtFrame->pts = gifFrameCount - 1;
                if (avcodec_send_frame(R.gifEncCtx, R.filtFrame) >= 0) {
                    AVPacket outPkt;
                    av_init_packet(&outPkt);
                    while (avcodec_receive_packet(R.gifEncCtx, &outPkt) >= 0) {
                        av_packet_rescale_ts(&outPkt, R.gifEncCtx->time_base,
                                             R.outVideoStream->time_base);
                        outPkt.stream_index = R.outVideoStream->index;
                        av_interleaved_write_frame(R.outFmtCtx, &outPkt);
                        av_packet_unref(&outPkt);
                    }
                }
                av_frame_unref(R.filtFrame);
            }
        }
    }

    // flush GIF 编码器
    avcodec_send_frame(R.gifEncCtx, nullptr);
    while (avcodec_receive_packet(R.gifEncCtx, R.pkt) >= 0) {
        av_packet_rescale_ts(R.pkt, R.gifEncCtx->time_base, R.outVideoStream->time_base);
        R.pkt->stream_index = R.outVideoStream->index;
        av_interleaved_write_frame(R.outFmtCtx, R.pkt);
        av_packet_unref(R.pkt);
    }

    ret = av_write_trailer(R.outFmtCtx);
    if (ret < 0) {
        logErr(ret, "write_trailer");
        return ret;
    }

    LOGI("GIF success frames=%lld", (long long) gifFrameCount);
    return 0;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_videoapp_utils_FFmpeg_clip(
        JNIEnv *env,
        jobject /*thiz*/,
        jstring jInput,
        jstring jOutput,
        jlong jStartMs,
        jlong jDurationMs,
        jboolean jReEncode
) {
    const char *inputPath = env->GetStringUTFChars(jInput, 0);
    const char *outputPath = env->GetStringUTFChars(jOutput, 0);

    int ret = 0;
    AVFormatContext *inFmtCtx = nullptr;
    AVFormatContext *outFmtCtx = nullptr;
    AVPacket *pkt = nullptr;

    int64_t startMs = (int64_t) jStartMs;
    int64_t durationMs = (int64_t) jDurationMs;
    bool reEncode = (bool) jReEncode;

    if (durationMs <= 0) {
        ret = AVERROR(EINVAL);
        return ret;
    }

    // 1. 打开输入
    if ((ret = avformat_open_input(&inFmtCtx, inputPath, nullptr, nullptr)) < 0) {
        return ret;
    }
    if ((ret = avformat_find_stream_info(inFmtCtx, nullptr)) < 0) {
        return ret;
    }

    // 2. 计算结束时间 (ms)
    int64_t totalDurationMs = (inFmtCtx->duration > 0)
                              ? inFmtCtx->duration / (AV_TIME_BASE / 1000)
                              : -1;
    if (totalDurationMs > 0 && startMs >= totalDurationMs) {
        ret = AVERROR(EINVAL);
        return ret;
    }
    int64_t endMs = startMs + durationMs;
    if (totalDurationMs > 0 && endMs > totalDurationMs) {
        endMs = totalDurationMs;
    }

    if (!reEncode) {
        // 3. 创建输出上下文
        if ((ret = avformat_alloc_output_context2(&outFmtCtx, nullptr, nullptr, outputPath)) < 0) {
            goto end;
        }

        // 4. 为每个输入流创建对应输出流（copy 参数）
        int streamCount = inFmtCtx->nb_streams;
        std::vector<int64_t> firstPts(streamCount, AV_NOPTS_VALUE);
        std::vector<int64_t> firstDts(streamCount, AV_NOPTS_VALUE);

        for (unsigned i = 0; i < inFmtCtx->nb_streams; ++i) {
            AVStream *inStream = inFmtCtx->streams[i];
            AVCodecParameters *inCodecPar = inStream->codecpar;

            AVStream *outStream = avformat_new_stream(outFmtCtx, nullptr);
            if (!outStream) {
                ret = AVERROR(ENOMEM);
                goto end;
            }

            // 复制参数
            if ((ret = avcodec_parameters_copy(outStream->codecpar, inCodecPar)) < 0) {
                goto end;
            }
            outStream->codecpar->codec_tag = 0; // 避免不兼容
        }

        // 5. 打开输出 IO
        if (!(outFmtCtx->oformat->flags & AVFMT_NOFILE)) {
            if ((ret = avio_open(&outFmtCtx->pb, outputPath, AVIO_FLAG_WRITE)) < 0) {
                goto end;
            }
        }

        // 6. 为快速裁剪 seek（关键帧）: 使用视频流或第一个可 seek 的流
        int videoStreamIndex = -1;
        for (unsigned i = 0; i < inFmtCtx->nb_streams; ++i) {
            if (inFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                videoStreamIndex = i;
                break;
            }
        }

        int seekStream = (videoStreamIndex >= 0) ? videoStreamIndex : 0;
        AVRational tb = inFmtCtx->streams[seekStream]->time_base;
        int64_t seekTarget = av_rescale_q(startMs, AVRational{1, 1000}, tb);

        // NOTE: 使用 AVSEEK_FLAG_BACKWARD 以确保落在之前的关键帧
        if ((ret = av_seek_frame(inFmtCtx, seekStream, seekTarget, AVSEEK_FLAG_BACKWARD)) < 0) {
            // 失败可忽略继续读（但起点可能不准确）
            av_log(nullptr, AV_LOG_WARNING, "Seek failed, continue without precise start.\n");
            avformat_seek_file(inFmtCtx, seekStream, INT64_MIN, 0, INT64_MAX, 0);
        }
//        avcodec_flush_buffers(inFmtCtx->streams[seekStream]->codec);

        // 7. 写文件头
        if ((ret = avformat_write_header(outFmtCtx, nullptr)) < 0) {
            goto end;
        }

        pkt = av_packet_alloc();
        if (!pkt) {
            ret = AVERROR(ENOMEM);
            goto end;
        }

        // 8. 主循环读取
        while ((ret = av_read_frame(inFmtCtx, pkt)) >= 0) {
            AVStream *inStream = inFmtCtx->streams[pkt->stream_index];
            AVStream *outStream = outFmtCtx->streams[pkt->stream_index];

            AVRational inTb = inStream->time_base;

            // 将当前 pkt pts 转为 ms 用于判断
            int64_t curMs = (pkt->pts == AV_NOPTS_VALUE)
                            ? -1
                            : av_rescale_q(pkt->pts, inTb, AVRational{1, 1000});

            // 丢弃起始前包
            if (curMs >= 0 && curMs < startMs) {
                av_packet_unref(pkt);
                continue;
            }
            // 超过结束时间
            if (curMs >= 0 && curMs > endMs) {
                av_packet_unref(pkt);
                break;
            }

            // 记录该流的初始 pts/dts 以归零
            if (firstPts[pkt->stream_index] == AV_NOPTS_VALUE && pkt->pts != AV_NOPTS_VALUE) {
                firstPts[pkt->stream_index] = pkt->pts;
            }
            if (firstDts[pkt->stream_index] == AV_NOPTS_VALUE && pkt->dts != AV_NOPTS_VALUE) {
                firstDts[pkt->stream_index] = pkt->dts;
            }

            // 重写 PTS/DTS
            if (pkt->pts != AV_NOPTS_VALUE && firstPts[pkt->stream_index] != AV_NOPTS_VALUE) {
                pkt->pts -= firstPts[pkt->stream_index];
            }
            if (pkt->dts != AV_NOPTS_VALUE && firstDts[pkt->stream_index] != AV_NOPTS_VALUE) {
                pkt->dts -= firstDts[pkt->stream_index];
            }

            // 再按输出流的时基进行缩放
            pkt->pts = (pkt->pts == AV_NOPTS_VALUE)
                       ? AV_NOPTS_VALUE
                       : av_rescale_q(pkt->pts, inTb, outStream->time_base);
            pkt->dts = (pkt->dts == AV_NOPTS_VALUE)
                       ? AV_NOPTS_VALUE
                       : av_rescale_q(pkt->dts, inTb, outStream->time_base);
            pkt->duration = av_rescale_q(pkt->duration, inTb, outStream->time_base);
            pkt->pos = -1;

            if (pkt->dts != AV_NOPTS_VALUE && pkt->pts != AV_NOPTS_VALUE && pkt->dts > pkt->pts) {
                // 避免错误顺序
                pkt->dts = pkt->pts;
            }

            if ((ret = av_interleaved_write_frame(outFmtCtx, pkt)) < 0) {
                av_packet_unref(pkt);
                goto end;
            }

            av_packet_unref(pkt);
        }

        if (ret == AVERROR_EOF) {
            ret = 0;
        }
        // 9. 写尾
        if (ret >= 0) {
            av_write_trailer(outFmtCtx);
        }
    } else {
        // TODO: reEncode = true 的情况（精确裁剪）
        // 先给出返回，后面你需要我再补完整 re-encode 版本的话再说。
        ret = AVERROR(ENOSYS); // 功能未实现
    }

    end:
    if (pkt) av_packet_free(&pkt);
    if (outFmtCtx) {
        if (!(outFmtCtx->oformat->flags & AVFMT_NOFILE) && outFmtCtx->pb) {
            avio_closep(&outFmtCtx->pb);
        }
        avformat_free_context(outFmtCtx);
    }
    if (inFmtCtx) {
        avformat_close_input(&inFmtCtx);
    }

    env->ReleaseStringUTFChars(jInput, inputPath);
    env->ReleaseStringUTFChars(jOutput, outputPath);
    return ret;
}

