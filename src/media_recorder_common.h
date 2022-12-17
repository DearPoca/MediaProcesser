#pragma once

#include <string>

struct MP4VideoRecorderStartParam {
    int width;
    int height;
    int fps;

    std::string filename;
};
