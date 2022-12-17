#pragma once

#include <string>

class MediaRecorder {
public:
    static MediaRecorder* CreateMP4VideoRecorder();

    virtual bool Start(void* param) = 0;
    virtual bool SendVideoFrame(void* data, int size) = 0;
    virtual bool SendAudioFrame(void* data, int size) = 0;
    virtual bool Stop() = 0;

    MediaRecorder(){};
    virtual ~MediaRecorder(){};
};
