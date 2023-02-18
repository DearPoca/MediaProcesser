#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
}

class ImageFilter {
public:
    static int drawText(AVFrame *frame_in, AVFrame *frame_out, const char *fontfile, int w, int h, int x, int y,
                        int fontsize, const char *str);

    static int cropAndScale(AVFrame *frame_in, AVFrame *frame_out, int l, int t, int r, int b, int w, int h);
};