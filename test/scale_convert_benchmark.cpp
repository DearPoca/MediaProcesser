#include <libyuv.h>

#include <chrono>
#include <cmath>
#include <functional>
#include <thread>

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

    uint8_t *srcSlice[1] = {src};
    int srcStride[1] = {src_width * 3};

    uint8_t *dstSlice[3] = {
        dst, dst + dst_width * dst_height,
        dst + dst_width * dst_height + (dst_height >> 1) * (dst_width >> 1)};
    int dstStride[3] = {dst_width, dst_width >> 1, dst_width >> 1};

    sws_scale(sws_ctx, srcSlice, srcStride, 0, src_height, dstSlice, dstStride);

    sws_freeContext(sws_ctx);
}

void FFmpegYUV420PToRGB24(uint8_t *src, int src_width, int src_height, uint8_t *dst,
                          int dst_width, int dst_height) {
    SwsContext *sws_ctx =
        sws_getContext(src_width, src_height, AV_PIX_FMT_YUV420P, dst_width, dst_height,
                       AV_PIX_FMT_RGB24, SWS_FAST_BILINEAR, 0, 0, 0);

    uint8_t *dstSlice[1] = {dst};
    int dstStride[1] = {dst_width * 3};

    uint8_t *srcSlice[3] = {
        src, src + src_width * src_height,
        src + src_width * src_height + (src_height >> 1) * (src_width >> 1)};
    int srcStride[3] = {src_width, src_width >> 1, src_width >> 1};

    sws_scale(sws_ctx, srcSlice, srcStride, 0, src_height, dstSlice, dstStride);

    sws_freeContext(sws_ctx);
}

int main(int argc, char **argv) {
    int width = 1920;
    int height = 1080;

    uint8_t *src = new uint8_t[width * height * 3];
    uint8_t *dst = new uint8_t[3840 * 2160 * 4];

    std::function<void(int, int, BenchmarkFunction, std::string)> benchMark =
        [&](int dst_width, int dst_height, BenchmarkFunction function,
            std::string func_name) {
            int64_t t1 = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::system_clock::now().time_since_epoch())
                             .count();

            for (int i = 0; i < 1000; i++) {
                function(src, width, height, dst, dst_width, dst_height);
            }

            int64_t t2 = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::system_clock::now().time_since_epoch())
                             .count();
            printf(
                "\033[1;33mFunction\033[0m [\033[0;32;34m%s\033[0m] from "
                "[\033[0;32m%dx%d\033[0m] to [\033[0;32m%dx%d\033[0m] used "
                "\033[0;36m%.3lfs\033[0m\n",
                func_name.c_str(), width, height, dst_width, dst_height,
                (t2 - t1) / 1000.0);
        };
#define RunBenchMark(dw, dh, f) benchMark(dw, dh, f, #f);

    // init rgb24 buffer
    for (int i = 0; i < width * height; i++) {
        int x = i % width;
        int y = i / width;
        if (x < width / 2) {
            if (y < height / 2) {
                src[i * 3 + 0] = 12;
                src[i * 3 + 1] = 233;
                src[i * 3 + 2] = 12;
            } else {
                src[i * 3 + 0] = 233;
                src[i * 3 + 1] = 12;
                src[i * 3 + 2] = 12;
            }
        } else if (x) {
            if (y < height / 2) {
                src[i * 3 + 0] = 233;
                src[i * 3 + 1] = 12;
                src[i * 3 + 2] = 233;
            } else {
                src[i * 3 + 0] = 12;
                src[i * 3 + 1] = 233;
                src[i * 3 + 2] = 233;
            }
        }
    }

    // RGB24 To YUV420P
    RunBenchMark(width, height, LibyuvRawToI420);
    RunBenchMark(width, height, FFmpegRGB24ToYUV420P);
    RunBenchMark(width / 4, height / 4, LibyuvRawToI420);
    RunBenchMark(width / 4, height / 4, FFmpegRGB24ToYUV420P);

    // init yuv420p buffer
    for (int i = 0; i < width * height; i++) {
        int x = i % width;
        int y = i / width;
        if (x < width / 2) {
            if (y < height / 2) {
                src[i] = 25;
            } else {
                src[i] = 178;
            }
        } else if (x) {
            if (y < height / 2) {
                src[i] = 234;
            } else {
                src[i] = 75;
            }
        }
    }
    for (int i = 0; i < (width >> 1); ++i) {
        for (int j = 0; j < (height >> 1); ++j) {
            // u
            src[width * height + j * (width >> 1) + i] = 254 * i / (width >> 1);
            // // v
            src[width * height + (width >> 1) * (height >> 1) + j * (width >> 1) + i] =
                254 * j / (height >> 1);
        }
    }

    // YUV420P To RGB24
    RunBenchMark(width, height, LibyuvI420ToRaw);
    RunBenchMark(width, height, FFmpegYUV420PToRGB24);
    RunBenchMark(width / 4, height / 4, LibyuvI420ToRaw);
    RunBenchMark(width / 4, height / 4, FFmpegYUV420PToRGB24);
    return 0;
}