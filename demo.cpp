#include <chrono>
#include <fstream>
#include <thread>

#include "logger.h"
#include "media_recorder_common.h"
#include "media_recorder_interface.h"
#include "ring_fifo.h"

int main(int argc, char** argv) {
    if (argc != 5) {
        printf("usage %s input_file width height output_file\n", argv[0]);
        exit(-1);
    }

    int width = atoi(argv[2]);
    int height = atoi(argv[3]);

    int frame_size = width * height * 3;
    char* src = (char*)malloc(frame_size * sizeof(char));

    MediaRecorder* recorder = MediaRecorder::CreateMP4VideoRecorder();
    MP4VideoRecorderStartParam param;
    param.width = width;
    param.height = height;
    param.fps = 30;
    param.filename = argv[4];
    recorder->Start(&param);

    for (int i = 0; i < 5; ++i) {
        int j = -1;
        std::ifstream ifs;
        ifs.open(argv[1], std::ios::in | std::ios::binary);
        if (!ifs.is_open()) {
            printf("read file failed\n");
            exit(-1);
        }
        while (ifs.read(src, frame_size).gcount() == frame_size) {
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
            recorder->SendVideoFrame(src, frame_size);
            j++;
        }
        ifs.clear();
        while (--j > 0) {
            ifs.seekg(j * frame_size, std::ios::beg);
            ifs.read(src, frame_size);
            recorder->SendVideoFrame(src, frame_size);
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
        }
    }

    recorder->Stop();

    return 0;
}