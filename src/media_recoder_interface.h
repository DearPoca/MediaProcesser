#pragma once

#include <string>

class MediaRecoder {
public:
    static MediaRecoder* CreateMP4VideoRecoder();

    virtual bool Start(void* param) = 0;
    virtual bool SendVideoFrame(void* data, int size) = 0;
    virtual bool SendAudioFrame(void* data, int size) = 0;
    virtual bool Stop() = 0;
};

struct MP4VideoRecoderStartParam {
    int width;
    int height;
    int fps;

    std::string filename;
};
