#include "image_filter.h"

#include "logger.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
}

int ImageFilter::drawText(AVFrame *frame_in, AVFrame *frame_out, const char *fontfile, int w, int h, int x, int y,
                          int fontsize, const char *str) {
    int ret;

    const AVFilter *buffersrc = avfilter_get_by_name("buffer");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");

    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs = avfilter_inout_alloc();

    AVFilterGraph *filter_graph = avfilter_graph_alloc();

    char args[256];
    snprintf(args, sizeof(args), "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d", w, h,
             AV_PIX_FMT_RGB24, 1, 25, 1, 1);

    AVFilterContext *buffersrc_ctx = nullptr;
    ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in", args, NULL, filter_graph);
    if (ret < 0) {
        log_error("Create filter src context failed");
        return -1;
    }

    AVFilterContext *buffersink_ctx;
    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out", NULL, NULL, filter_graph);
    if (ret < 0) {
        log_error("Create filter sink context failed");
        return -2;
    }

    outputs->name = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx = 0;
    outputs->next = NULL;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx = 0;
    inputs->next = NULL;
    char filter_desrc[2048] = {0};
    snprintf(filter_desrc, sizeof(filter_desrc),
             "drawtext=fontfile=%s:"
             "DejaVuSansCondensed.ttf:fontcolor=white:fontsize=%d:x=%d:y=%d:text='%s'",
             fontfile, fontsize, x, y, str);
    if (avfilter_graph_parse_ptr(filter_graph, filter_desrc, &inputs, &outputs, NULL) < 0) {
        log_error("add text failed");
        return -3;
    }

    if (avfilter_graph_config(filter_graph, NULL) < 0) {
        log_error("filter config error");
        return -4;
    }

    if (av_buffersrc_add_frame(buffersrc_ctx, frame_in) < 0) {
        log_error("add src frame failed");
        return -5;
    }

    if (av_buffersink_get_frame(buffersink_ctx, frame_out) < 0) {
        log_error("add sink frame failed");
        return -6;
    }

    avfilter_inout_free(&outputs);
    avfilter_inout_free(&inputs);
    avfilter_graph_free(&filter_graph);
    return 0;
}

int ImageFilter::cropAndScale(AVFrame *frame_in, AVFrame *frame_out, int l, int t, int r, int b, int w, int h) {
    int ret;

    const AVFilter *buffersrc = avfilter_get_by_name("buffer");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");

    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs = avfilter_inout_alloc();

    AVFilterGraph *filter_graph = avfilter_graph_alloc();

    char args[256];
    snprintf(args, sizeof(args), "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d", w, h,
             AV_PIX_FMT_RGB24, 1, 25, 1, 1);

    AVFilterContext *buffersrc_ctx = nullptr;
    ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in", args, NULL, filter_graph);
    if (ret < 0) {
        log_error("Create filter src context failed");
        return -1;
    }

    AVFilterContext *buffersink_ctx;
    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out", NULL, NULL, filter_graph);
    if (ret < 0) {
        log_error("Create filter sink context failed");
        return -2;
    }

    outputs->name = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx = 0;
    outputs->next = NULL;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx = 0;
    inputs->next = NULL;
    char filter_desrc[2048] = {0};
    snprintf(filter_desrc, sizeof(filter_desrc), "crop=%d:%d:%d:%d,scale=%d:%d", r - l, b - t, l, t, w, h);
    if (avfilter_graph_parse_ptr(filter_graph, filter_desrc, &inputs, &outputs, NULL) < 0) {
        log_error("add text failed");
        return -3;
    }

    if (avfilter_graph_config(filter_graph, NULL) < 0) {
        log_error("filter config error");
        return -4;
    }

    if (av_buffersrc_add_frame(buffersrc_ctx, frame_in) < 0) {
        log_error("add src frame failed");
        return -5;
    }

    if (av_buffersink_get_frame(buffersink_ctx, frame_out) < 0) {
        log_error("add sink frame failed");
        return -6;
    }

    avfilter_inout_free(&outputs);
    avfilter_inout_free(&inputs);
    avfilter_graph_free(&filter_graph);
    return 0;
}