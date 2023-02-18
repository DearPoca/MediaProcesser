#include <libyuv.h>

#include <chrono>
#include <deque>
#include <fstream>
#include <thread>

#include "image_filter.h"
#include "logger.h"
#include "media_decoder_common.h"
#include "media_decoder_interface.h"
#include "media_recorder_common.h"
#include "media_recorder_interface.h"
#include "ring_fifo.h"

extern "C" {
#include <libavutil/avutil.h>
}

int main(int argc, char **argv) {
    if (argc < 4) {
        printf("usage %s input_file output_file fontfile\n", argv[0]);
        exit(-1);
    }
    MediaDecoder *dec = MediaDecoder::CreateVideoDecoder();
    VideoDecoderStartParam dec_param;
    dec_param.filename = argv[1];
    dec->Start(&dec_param);
    int width = dec_param.width;
    int height = dec_param.height;

    MediaRecorder *recorder = MediaRecorder::CreateMP4VideoRecorder();
    MP4VideoRecorderStartParam rec_param;
    rec_param.width = width;
    rec_param.height = height;
    rec_param.fps = dec_param.fps;
    rec_param.filename = argv[2];
    recorder->Start(&rec_param);

    AVFrame *frame_a = av_frame_alloc();
    AVFrame *frame_b = av_frame_alloc();
    frame_a->format = AV_PIX_FMT_RGB24;
    frame_a->width = width;
    frame_a->height = height;
    av_frame_get_buffer(frame_a, 0);
    frame_b->format = AV_PIX_FMT_RGB24;
    frame_b->width = width;
    frame_b->height = height;
    av_frame_get_buffer(frame_b, 0);

    const char *strs = "@#$&MA?ebi!;+_*-,^~.    ";
    int len_strs = strlen(strs);
    int width_reduction = 6;
    int height_reduction = 10;
    char *row_char = new char[width / width_reduction + 1];

    MediaDecoder::Frame *frame = nullptr;
    dec->InitFrame(&frame);
    int yuv_frame_size = width * height / 2 + width * height;
    uint8_t *yuv_tmp = (uint8_t *)malloc(yuv_frame_size * sizeof(uint8_t));

    int cnt = 0;

    while (true) {
        if (!dec->ReadFrame(frame)) {
            break;
        }
        libyuv::RAWToI420(frame->data, width * 3, yuv_tmp, width, yuv_tmp + width * height, (width + 1) / 2,
                          yuv_tmp + width * height + ((width + 1) / 2) * ((height + 1) / 2), (width + 1) / 2, width,
                          height);
        memset(frame_a->data[0], 0, frame_a->linesize[0] * frame_a->height);
        memset(frame_b->data[0], 0, frame_b->linesize[0] * frame_b->height);
        for (int row = 0; row < height / height_reduction; ++row) {
            for (int col = 0; col < width / width_reduction; ++col) {
                int sum = 0;
                for (int i = 0; i < height_reduction; ++i) {
                    for (int j = 0; j < width_reduction; ++j) {
                        sum += yuv_tmp[(row * height_reduction + i) * width + col * width_reduction + j];
                    }
                }
                int index = sum * len_strs / height_reduction / width_reduction / 256.0;
                log_debug("sum: %d, index: %d", sum, index);
                row_char[col] = strs[index];
            }
            row_char[width / width_reduction] = '\0';
            log_debug("row_char: %s", row_char);
            ImageFilter::drawText(frame_a, frame_b, argv[3], width, height, 0, row * height_reduction, height_reduction,
                                  row_char);
            AVFrame *av_frame_tmp = frame_a;
            frame_a = frame_b;
            frame_b = av_frame_tmp;
        }
        // ImageFilter::cropAndScale(frame_a, frame_b, 0, 0, height, height, width, height);
        // AVFrame *av_frame_tmp = frame_a;
        // frame_a = frame_b;
        // frame_b = av_frame_tmp;
        log_info("frame cnt: %d", cnt++);
        recorder->SendVideoFrameBlock(frame_a->data[0], frame_a->linesize[0] * frame_a->height);
    }
    recorder->Stop();

    return 0;
}