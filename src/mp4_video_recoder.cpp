#include "media_recoder_interface.h"

class MP4VideoRecoder : public MediaRecoder {
public:
    virtual bool Start(void* param);
    virtual bool SendVideoFrame(void* data, int size);
    virtual bool SendAudioFrame(void* data, int size);
    virtual bool Stop();

private:
};

MediaRecoder* MediaRecoder::CreateMP4VideoRecoder() { return new MP4VideoRecoder(); }

bool MP4VideoRecoder::Start(void* param) {
    MP4VideoRecoderStartParam* start_param = reinterpret_cast<MP4VideoRecoderStartParam*>(param);
    return true;
}

bool MP4VideoRecoder::SendVideoFrame(void* data, int size) { return true; }

bool MP4VideoRecoder::SendAudioFrame(void* data, int size) { return false; }

bool MP4VideoRecoder::Stop() { return true; }