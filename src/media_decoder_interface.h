#pragma once

#include <string>

extern "C" {
#include <libavutil/frame.h>
}

class MediaDecoder {
public:
    static MediaDecoder* CreateVideoDecoder();

    virtual MediaDecoderStartRet Start(void* param) = 0;
    virtual bool InitFrame(AVFrame** frame) = 0;
    virtual bool ReadFrame(AVFrame* frame) = 0;

    MediaDecoder(){};
    virtual ~MediaDecoder(){};
};