#include "poca_str.h"

std::string poca_err2str(int errnum) {
    char tmp[AV_ERROR_MAX_STRING_SIZE] = {0};
    return std::string(std::move(av_make_error_string(tmp, AV_ERROR_MAX_STRING_SIZE, errnum))).c_str();
}

std::string poca_ts2timestr(int64_t ts, AVRational* tb) {
    char tmp[AV_TS_MAX_STRING_SIZE] = {0};
    return std::string(std::move(av_ts_make_time_string(tmp, ts, tb))).c_str();
}

std::string poca_ts2str(int64_t ts) {
    char tmp[AV_TS_MAX_STRING_SIZE] = {0};
    return std::string(std::move(av_ts_make_string(tmp, ts))).c_str();
}