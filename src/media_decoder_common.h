#pragma once

#include <string>

struct VideoDecoderStartParam {
    std::string filename;
};

struct MediaDecoderStartRet {
    bool success = false;

    // for video:
    int width = 0;
    int height = 0;
    int fps = 0;
};
