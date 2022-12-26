#include "poca_str.h"

std::string poca_err2str(int errnum) {
    char tmp[AV_ERROR_MAX_STRING_SIZE] = {0};
    av_strerror(errnum, tmp, AV_ERROR_MAX_STRING_SIZE);
    return std::string(tmp);
}

std::string poca_ts2timestr(int64_t ts, AVRational* tb) {
    char tmp[AV_TS_MAX_STRING_SIZE] = {0};
    av_ts_make_time_string(tmp, ts, tb);
    return std::string(tmp);
}

std::string poca_ts2str(int64_t ts) {
    char tmp[AV_TS_MAX_STRING_SIZE] = {0};
    av_ts_make_string(tmp, ts);
    return std::string(tmp);
}