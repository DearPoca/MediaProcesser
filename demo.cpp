#include <chrono>
#include <fstream>
#include <thread>

#include "logger.h"
#include "media_recorder_interface.h"
#include "ring_fifo.h"

int main1() {
    RingFIFO<int> q(5);
    for (int i = 0; i < 5; ++i) {
        bool ret = q.PutNoWait(1);
        if (ret) {
            log_debug("success");
        } else {
            log_debug("fail");
        }
    }
    for (int i = 0; i < 5; ++i) {
        int a = q.Get();
        log_debug("%d", a);
    }
}

int main(int argc, char** argv) {
    if (argc != 3) {
        printf("usage %s input_file output_file\n", argv[0]);
        exit(-1);
    }

    int frame_size = 1280 * 720 * 4;
    char* src = (char*)malloc(frame_size * sizeof(char));

    std::ifstream ifs;
    ifs.open(argv[1], std::ios::in | std::ios::binary);
    if (!ifs.is_open()) {
        printf("read file failed\n");
        exit(-1);
    }

    MediaRecorder* recoder = MediaRecorder::CreateMP4VideoRecoder();
    MP4VideoRecorderStartParam param;
    param.width = 1280;
    param.height = 720;
    param.fps = 30;
    param.filename = argv[2];
    recoder->Start(&param);

    while (ifs.read(src, frame_size).gcount() == frame_size) {
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        recoder->SendVideoFrame(src, frame_size);
    }

    recoder->Stop();

    return 0;
}