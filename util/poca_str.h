#pragma once

#include <string>

extern "C" {
#include <libavutil/timestamp.h>
}

std::string poca_err2str(int errnum);

std::string poca_ts2timestr(int64_t ts, AVRational* tb);

std::string poca_ts2str(int64_t ts);