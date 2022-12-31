#pragma once

#include <string>

extern "C" {
#include "libavutil/avutil.h"
#include "libavutil/pixfmt.h"
}

class MediaDecoder {
public:
    struct Frame {
        uint8_t* data;
        int len;
        int64_t timestamp;
        enum AVPixelFormat pix_fmt;
        enum AVMediaType stream_type;
    };

    static MediaDecoder* CreateVideoDecoder();

    virtual bool Start(void* param) = 0;
    virtual void InitFrame(Frame** frame) = 0;
    virtual bool ReadFrame(Frame* frame) = 0;

    MediaDecoder(){};
    virtual ~MediaDecoder(){};
};