#pragma once

#include <string>

struct VideoDecoderStartParam {
    std::string filename;

    int width;
    int height;
    int fps;
};
