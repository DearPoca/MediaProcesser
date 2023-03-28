#include <libyuv.h>

#include <chrono>
#include <deque>
#include <fstream>
#include <functional>
#include <thread>

#include "logger.h"
#include "media_decoder_common.h"
#include "media_decoder_interface.h"
#include "media_recorder_common.h"
#include "media_recorder_interface.h"
#include "ring_fifo.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
#include <libswscale/swscale.h>
}

typedef void (*BenchmarkFunction)(uint8_t *, int, int, uint8_t *, int, int);

void LibyuvRawToI420(uint8_t *src, int src_width, int src_height, uint8_t *dst,
                     int dst_width, int dst_height) {
    if (src_width == dst_width && src_height == dst_height) {
        libyuv::RAWToI420(
            src, src_width * 3, dst, src_width, dst + src_width * src_height,
            src_width >> 1,
            dst + src_width * src_height + (src_width >> 1) * (src_height >> 1),
            src_width >> 1, src_width, src_height);
    } else {
        static uint8_t *tmp_buf = new uint8_t[3840 * 2160 * 4];
        libyuv::RAWToI420(
            src, src_width * 3, tmp_buf, src_width, tmp_buf + src_width * src_height,
            src_width >> 1,
            tmp_buf + src_width * src_height + (src_width >> 1) * (src_height >> 1),
            src_width >> 1, src_width, src_height);
        libyuv::I420Scale(
            tmp_buf, src_width, tmp_buf + src_width * src_height, src_width >> 1,
            tmp_buf + src_width * src_height + (src_width >> 1) * (src_height >> 1),
            src_width >> 1, src_width, src_height, dst, dst_width,
            dst + dst_width * dst_height, dst_width >> 1,
            dst + dst_width * dst_height + (dst_height >> 1) * (dst_width >> 1),
            dst_width >> 1, dst_width, dst_height, libyuv::kFilterNone);
    }
}

void LibyuvI420ToRaw(uint8_t *src, int src_width, int src_height, uint8_t *dst,
                     int dst_width, int dst_height) {
    if (src_width == dst_width && src_height == dst_height) {
        libyuv::I420ToRAW(
            src, src_width, src + src_width * src_height, src_width >> 1,
            src + src_width * src_height + (src_width >> 1) * (src_height >> 1),
            src_width >> 1, dst, dst_width * 3, src_width, src_height);
    } else {
        static uint8_t *tmp_buf = new uint8_t[3840 * 2160 * 4];
        libyuv::I420Scale(
            src, src_width, src + src_width * src_height, src_width >> 1,
            src + src_width * src_height + (src_width >> 1) * (src_height >> 1),
            src_width >> 1, src_width, src_height, tmp_buf, dst_width,
            tmp_buf + dst_width * dst_height, dst_width >> 1,
            tmp_buf + dst_width * dst_height + (dst_height >> 1) * (dst_width >> 1),
            dst_width >> 1, dst_width, dst_height, libyuv::kFilterNone);
        libyuv::I420ToRAW(
            tmp_buf, dst_width, tmp_buf + dst_width * dst_height, dst_width >> 1,
            tmp_buf + dst_width * dst_height + (dst_height >> 1) * (dst_width >> 1),
            dst_width >> 1, dst, dst_width * 3, dst_width, dst_height);
    }
}

void FFmpegRGB24ToYUV420P(uint8_t *src, int src_width, int src_height, uint8_t *dst,
                          int dst_width, int dst_height) {
    SwsContext *sws_ctx =
        sws_getContext(src_width, src_height, AV_PIX_FMT_RGB24, dst_width, dst_height,
                       AV_PIX_FMT_YUV420P, SWS_FAST_BILINEAR, 0, 0, 0);
    AVFrame *src_frame;
    src_frame = av_frame_alloc();
    src_frame->width = src_width;
    src_frame->height = src_height;
    src_frame->format = AV_PIX_FMT_RGB24;
    AVFrame *dst_frame;
    av_frame_get_buffer(src_frame, 0);

    dst_frame = av_frame_alloc();
    dst_frame->width = dst_width;
    dst_frame->height = dst_height;
    dst_frame->format = AV_PIX_FMT_YUV420P;
    av_frame_get_buffer(dst_frame, 0);

    memcpy(src_frame->data[0], src, src_width * src_height * 3);
    sws_scale(sws_ctx, src_frame->data, src_frame->linesize, 0, src_frame->height,
              dst_frame->data, dst_frame->linesize);
    memcpy(dst, dst_frame->data[0], dst_width * dst_height);
    memcpy(dst + dst_width * dst_height, dst_frame->data[0],
           (dst_width >> 1) * (dst_height >> 1));
    memcpy(dst + dst_width * dst_height + (dst_width >> 1) * (dst_height >> 1),
           dst_frame->data[0], (dst_width >> 1) * (dst_height >> 1));
    av_frame_free(&src_frame);
    av_frame_free(&dst_frame);
    sws_freeContext(sws_ctx);
}

void FFmpegYUV420PToRGB24(uint8_t *src, int src_width, int src_height, uint8_t *dst,
                          int dst_width, int dst_height) {
    SwsContext *sws_ctx =
        sws_getContext(src_width, src_height, AV_PIX_FMT_YUV420P, dst_width, dst_height,
                       AV_PIX_FMT_RGB24, SWS_FAST_BILINEAR, 0, 0, 0);
    AVFrame *src_frame;
    src_frame = av_frame_alloc();
    src_frame->width = src_width;
    src_frame->height = src_height;
    src_frame->format = AV_PIX_FMT_YUV420P;
    AVFrame *dst_frame;
    av_frame_get_buffer(src_frame, 0);

    dst_frame = av_frame_alloc();
    dst_frame->width = dst_width;
    dst_frame->height = dst_height;
    dst_frame->format = AV_PIX_FMT_RGB24;
    av_frame_get_buffer(dst_frame, 0);

    memcpy(src_frame->data[0], src, src_width * src_height);
    memcpy(src_frame->data[1], src + src_width * src_height,
           (src_width >> 1) * (src_height >> 1));
    memcpy(src_frame->data[2],
           src + src_width * src_height + (src_width >> 1) * (src_height >> 1),
           (src_width >> 1) * (src_height >> 1));
    sws_scale(sws_ctx, src_frame->data, src_frame->linesize, 0, src_frame->height,
              dst_frame->data, dst_frame->linesize);
    memcpy(dst, dst_frame->data[0], dst_width * dst_height * 3);
    av_frame_free(&src_frame);
    av_frame_free(&dst_frame);
    sws_freeContext(sws_ctx);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("usage %s input_file\n", argv[0]);
        exit(-1);
    }
    MediaDecoder *dec = MediaDecoder::CreateVideoDecoder();
    VideoDecoderStartParam dec_param;
    dec_param.filename = argv[1];
    MediaDecoderStartRet dec_ret = dec->Start(&dec_param);

    AVFrame *frame = nullptr;
    dec->InitFrame(&frame);
    dec->ReadFrame(frame);

    uint8_t *src = new uint8_t[dec_ret.width * dec_ret.height * 4];
    uint8_t *dst = new uint8_t[3840 * 2160 * 4];

    std::function<void(int, int, BenchmarkFunction, std::string)> benchMark =
        [&](int dst_width, int dst_height, BenchmarkFunction function,
            std::string func_name) {
            int64_t t1 = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::system_clock::now().time_since_epoch())
                             .count();

            for (int i = 0; i < 1000; i++) {
                function(src, dec_ret.width, dec_ret.height, dst, dst_width, dst_height);
            }

            int64_t t2 = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::system_clock::now().time_since_epoch())
                             .count();
            printf(
                "\033[1;33mFunction\033[0m [\033[0;32;34m%s\033[0m] from "
                "[\033[0;32m%dx%d\033[0m] to [\033[0;32m%dx%d\033[0m] used "
                "\033[0;36m%.3lfs\033[0m\n",
                func_name.c_str(), dec_ret.width, dec_ret.height, dst_width, dst_height,
                (t2 - t1) / 1000.0);
        };
#define RunBenchMark(dw, dh, f) benchMark(dw, dh, f, #f);

    // init rgb24 src buffer
    memcpy(src, frame->data[0], frame->width * frame->height * 3);

    RunBenchMark(dec_ret.width, dec_ret.height, LibyuvRawToI420);
    RunBenchMark(dec_ret.width, dec_ret.height, FFmpegRGB24ToYUV420P);
    RunBenchMark(dec_ret.width / 4, dec_ret.height / 4, LibyuvRawToI420);
    RunBenchMark(dec_ret.width / 4, dec_ret.height / 4, FFmpegRGB24ToYUV420P);

    // init yuv420p src buffer
    libyuv::RAWToI420(
        frame->data[0], frame->linesize[0], src, frame->width,
        src + frame->width * frame->height, frame->width >> 1,
        src + frame->width * frame->height + (frame->width / 2) * (frame->height / 2),
        frame->width >> 1, frame->width, frame->height);

    RunBenchMark(dec_ret.width, dec_ret.height, LibyuvI420ToRaw);
    RunBenchMark(dec_ret.width, dec_ret.height, FFmpegYUV420PToRGB24);
    RunBenchMark(dec_ret.width / 4, dec_ret.height / 4, LibyuvI420ToRaw);
    RunBenchMark(dec_ret.width / 4, dec_ret.height / 4, FFmpegYUV420PToRGB24);
    return 0;
}