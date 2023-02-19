#include <libyuv.h>

#include <thread>

#include "logger.h"
#include "media_decoder_common.h"
#include "media_decoder_interface.h"
#include "poca_str.h"
#include "ring_fifo.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

class VideoDecoder : public MediaDecoder {
public:
    virtual MediaDecoderStartRet Start(void* param) override;
    virtual bool ReadFrame(AVFrame* frame) override;
    virtual bool InitFrame(AVFrame** frame) override;

    virtual ~VideoDecoder() override;

private:
    int width_;
    int height_;
    enum AVPixelFormat src_pix_fmt_;

    std::string src_filename_;

    static const int buffer_size_;

    AVFormatContext* src_fmt_ctx_;
    int video_stream_idx_;
    AVStream* video_stream_;
    AVCodecContext* video_decode_ctx_;
    AVPacket* src_video_pkt_;

    RingFIFO<AVFrame*>* ring_fifo_av_frame_full_;
    RingFIFO<AVFrame*>* ring_fifo_av_frame_empty_;

    std::thread worker_thread_;

    bool InitAVContexts();
    void ReadPacketAndDecode();
    int DecodePacket(AVCodecContext* dec, AVPacket* pkt);
};

const int VideoDecoder::buffer_size_ = 10;

MediaDecoder* MediaDecoder::CreateVideoDecoder() { return new VideoDecoder(); }

VideoDecoder::~VideoDecoder() {}

bool VideoDecoder::InitFrame(AVFrame** frame) {
    if (*frame == nullptr) {
        *frame = av_frame_alloc();
        if (!(*frame)) return false;
    }

    (*frame)->format = AV_PIX_FMT_RGB24;
    (*frame)->width = width_;
    (*frame)->height = height_;

    /* allocate the buffers for the frame data */
    if (av_frame_get_buffer(*frame, 0) < 0) {
        log_error("Could not allocate frame data.");
        return false;
    }
    return true;
}

bool VideoDecoder::InitAVContexts() {
    int ret;
    AVStream* st;
    const AVCodec* decoder = nullptr;

    log_info("Init av contexts begin");

    ret = av_find_best_stream(src_fmt_ctx_, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);

    if (ret < 0) {
        log_error("Could not find video stream in input file '%s'", src_filename_.c_str());
        return false;
    }

    video_stream_idx_ = ret;
    st = src_fmt_ctx_->streams[video_stream_idx_];

    decoder = avcodec_find_decoder(st->codecpar->codec_id);
    if (!decoder) {
        log_error("Failed to find video codec");
        return false;
    }

    video_decode_ctx_ = avcodec_alloc_context3(decoder);
    if (!video_decode_ctx_) {
        log_error("Failed to allocate the video codec context");
        return false;
    }

    if ((ret = avcodec_parameters_to_context(video_decode_ctx_, st->codecpar)) < 0) {
        log_error("Failed to copy video codec parameters to decoder context");
        return false;
    }

    if ((ret = avcodec_open2(video_decode_ctx_, decoder, NULL)) < 0) {
        log_error("Failed to open video codec");
        return false;
    }

    src_video_pkt_ = av_packet_alloc();
    if (!src_video_pkt_) {
        log_error("Could not allocate AVPacket");
        return false;
    }

    video_stream_ = src_fmt_ctx_->streams[video_stream_idx_];
    width_ = video_decode_ctx_->width;
    height_ = video_decode_ctx_->height;
    src_pix_fmt_ = video_decode_ctx_->pix_fmt;

    log_info("Init av contexts success");

    return true;
}

MediaDecoderStartRet VideoDecoder::Start(void* param) {
    MediaDecoderStartRet ret;
    if (param == nullptr) {
        log_error("Start param is nullptr");
        return ret;
    }
    VideoDecoderStartParam* start_param = reinterpret_cast<VideoDecoderStartParam*>(param);

    if (avformat_open_input(&src_fmt_ctx_, start_param->filename.c_str(), NULL, NULL) < 0) {
        log_error("Could not open source file %s", start_param->filename.c_str());
        return ret;
    }

    if (avformat_find_stream_info(src_fmt_ctx_, NULL) < 0) {
        log_error("Could not find stream information");
        return ret;
    }

    if (!InitAVContexts()) {
        return ret;
    }

    ring_fifo_av_frame_full_ = new RingFIFO<AVFrame*>(buffer_size_);
    ring_fifo_av_frame_empty_ = new RingFIFO<AVFrame*>(buffer_size_);

    for (int i = 0; i < buffer_size_; ++i) {
        AVFrame* frame;

        frame = av_frame_alloc();
        if (!frame) return ret;

        frame->format = src_pix_fmt_;
        frame->width = width_;
        frame->height = height_;

        /* allocate the buffers for the frame data */
        if (av_frame_get_buffer(frame, 0) < 0) {
            log_error("Could not allocate frame data.");
            return ret;
        }
        ring_fifo_av_frame_empty_->Put(frame);
    }

    worker_thread_ = std::thread(&VideoDecoder::ReadPacketAndDecode, this);
    ret.width = width_;
    ret.height = height_;
    ret.fps = av_q2d(src_fmt_ctx_->streams[video_stream_idx_]->avg_frame_rate);

    return ret;
}

int VideoDecoder::DecodePacket(AVCodecContext* dec, AVPacket* pkt) {
    int ret = 0;

    ret = avcodec_send_packet(dec, pkt);
    if (ret < 0) {
        log_error("Error submitting a packet for decoding (%s)", poca_err2str(ret));
        return ret;
    }

    while (ret >= 0) {
        AVFrame* frame = ring_fifo_av_frame_empty_->Get();
        ret = avcodec_receive_frame(dec, frame);
        if (ret < 0) {
            if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) {
                ring_fifo_av_frame_empty_->Put(frame);
                return 0;
            }
            log_error("Error during decoding (%s)", poca_err2str(ret));
            return ret;
        }
        frame->time_base = src_fmt_ctx_->streams[video_stream_idx_]->time_base;
        ring_fifo_av_frame_full_->Put(frame);
    }
    return 0;
}

void VideoDecoder::ReadPacketAndDecode() {
    int ret = 0;
    AVRational* time_base = &src_fmt_ctx_->streams[video_stream_idx_]->time_base;
    while (av_read_frame(src_fmt_ctx_, src_video_pkt_) >= 0) {
        log_debug("pts:%s pts_time:%s stream_index:%d", poca_ts2str(src_video_pkt_->pts).c_str(),
                  poca_ts2timestr(src_video_pkt_->pts, time_base).c_str(), src_video_pkt_->stream_index);
        if (src_video_pkt_->stream_index == video_stream_idx_) {
            ret = DecodePacket(video_decode_ctx_, src_video_pkt_);
        }
        av_packet_unref(src_video_pkt_);
        if (ret < 0) break;
    }
    DecodePacket(video_decode_ctx_, nullptr);
    log_info("Decode finished");
    ring_fifo_av_frame_full_->Put(nullptr);
}

bool VideoDecoder::ReadFrame(AVFrame* frame) {
    if (frame == nullptr) {
        return false;
    }

    AVFrame* av_frame = ring_fifo_av_frame_full_->Get();

    if (av_frame == nullptr) {
        return false;
    }

    libyuv::I420ToRAW(av_frame->data[0], width_, av_frame->data[1], width_ >> 1, av_frame->data[2], width_ >> 1,
                      frame->data[0], width_ * 3, width_, height_);

    frame->pts = av_frame->pts * 1000 * av_q2d(av_frame->time_base);
    frame->time_base = (AVRational){1, 1000};

    ring_fifo_av_frame_empty_->Put(av_frame);

    return true;
}