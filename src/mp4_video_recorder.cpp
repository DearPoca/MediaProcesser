#include <libyuv.h>

#include <thread>

#include "logger.h"
#include "media_recorder_common.h"
#include "media_recorder_interface.h"
#include "poca_str.h"
#include "ring_fifo.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
}

class MP4VideoRecorder : public MediaRecorder {
public:
    virtual bool Start(void* param) override;
    virtual bool SendVideoFrame(void* data, int size) override;
    virtual bool SendAudioFrame(void* data, int size) override;
    virtual bool SendVideoFrameBlock(void* data, int size) override;
    virtual bool SendAudioFrameBlock(void* data, int size) override;
    virtual bool Stop() override;

    virtual ~MP4VideoRecorder() override;

private:
    int fps_;
    int width_;
    int height_;

    std::string filename_;

    static const int buffer_size_;
    static const AVPixelFormat output_pix_fmt_;
    static const char* const format_name_;
    static const AVCodecID codec_id_;

    RingFIFO<AVFrame*>* ring_fifo_av_frame_full_;
    RingFIFO<AVFrame*>* ring_fifo_av_frame_empty_;

    AVFormatContext* dst_fmt_ctx_;
    AVStream* dst_video_stream_;
    AVCodecContext* encoder_ctx_;
    AVPacket* dst_video_pkt_;

    std::thread worker_thread_;

    bool InitAVContexts();
    void EncodeAndWriteFrame();
    bool WriteFrame(AVFrame* frame);
};

const int MP4VideoRecorder::buffer_size_ = 10;
const AVPixelFormat MP4VideoRecorder::output_pix_fmt_ = AV_PIX_FMT_YUV420P;
const char* const MP4VideoRecorder::format_name_ = "mp4";
const AVCodecID MP4VideoRecorder::codec_id_ = AV_CODEC_ID_H264;

MediaRecorder* MediaRecorder::CreateMP4VideoRecorder() { return new MP4VideoRecorder(); }

MP4VideoRecorder::~MP4VideoRecorder() {}

bool MP4VideoRecorder::InitAVContexts() {
    int ret;
    AVCodec* encoder;
    AVDictionary* opt;
    log_info("Init av contexts begin");

    if (avformat_alloc_output_context2(&dst_fmt_ctx_, nullptr, format_name_, filename_.c_str()) < 0) {
        log_error("Alloc avformat output ctx failed");
        return false;
    }

    encoder = const_cast<AVCodec*>(avcodec_find_encoder(codec_id_));
    if (!encoder) {
        log_error("Necessary encoder not found");
        return false;
    }

    dst_video_pkt_ = av_packet_alloc();
    if (!dst_video_pkt_) {
        log_error("Could not allocate AVPacket");
        return false;
    }

    dst_video_stream_ = avformat_new_stream(dst_fmt_ctx_, NULL);
    if (!dst_video_stream_) {
        log_error("Alloc output stream failed");
        return false;
    }
    dst_video_stream_->time_base = (AVRational){1, fps_};

    encoder_ctx_ = avcodec_alloc_context3(encoder);
    if (!encoder_ctx_) {
        log_error("Failed to allocate the encoder context");
        return false;
    }

    encoder_ctx_->height = height_;
    encoder_ctx_->width = width_;
    encoder_ctx_->pix_fmt = output_pix_fmt_;
    encoder_ctx_->time_base = dst_video_stream_->time_base;
    if (dst_fmt_ctx_->oformat->flags & AVFMT_GLOBALHEADER) {
        encoder_ctx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    opt = 0;
    av_dict_set(&opt, "threads", "4", 0);
    ret = avcodec_open2(encoder_ctx_, encoder, &opt);
    av_dict_free(&opt);
    if (ret < 0) {
        log_error("Could not open video codec: %s", poca_err2str(ret));
        return false;
    }

    if (avcodec_parameters_from_context(dst_video_stream_->codecpar, encoder_ctx_) < 0) {
        log_error("Failed to copy encoder parameters to output");
        return false;
    }

    av_dump_format(dst_fmt_ctx_, 0, filename_.c_str(), 1);

    if (!(dst_fmt_ctx_->flags & AVFMT_NOFILE)) {
        ret = avio_open(&dst_fmt_ctx_->pb, filename_.c_str(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            log_error("Could not open '%s': %s", filename_.c_str(), poca_err2str(ret));
            return false;
        }
    }

    /* Write the stream header, if any. */
    ret = avformat_write_header(dst_fmt_ctx_, &opt);
    if (ret < 0) {
        log_error("Error occurred when opening output file: %s", poca_err2str(ret));
        return false;
    }

    log_info("Init av contexts success");
    log_debug("encoder context time base: {%d / %d}, stream context time base: {%d / %d}", encoder_ctx_->time_base.num,
              encoder_ctx_->time_base.den, dst_video_stream_->time_base.num, dst_video_stream_->time_base.den);
    return true;
}

bool MP4VideoRecorder::Start(void* param) {
    if (param == nullptr) {
        log_error("Start param is nullptr");
        return false;
    }
    MP4VideoRecorderStartParam* start_param = reinterpret_cast<MP4VideoRecorderStartParam*>(param);
    fps_ = start_param->fps;
    width_ = start_param->width;
    height_ = start_param->height;
    filename_ = start_param->filename;

    ring_fifo_av_frame_full_ = new RingFIFO<AVFrame*>(buffer_size_);
    ring_fifo_av_frame_empty_ = new RingFIFO<AVFrame*>(buffer_size_);

    for (int i = 0; i < buffer_size_; ++i) {
        AVFrame* frame;
        int ret;

        frame = av_frame_alloc();
        if (!frame) return false;

        frame->format = output_pix_fmt_;
        frame->width = width_;
        frame->height = height_;

        /* allocate the buffers for the frame data */
        ret = av_frame_get_buffer(frame, 0);
        if (ret < 0) {
            log_error("Could not allocate frame data.");
            return false;
        }
        ring_fifo_av_frame_empty_->Put(frame);
    }

    if (!InitAVContexts()) return false;

    worker_thread_ = std::thread(&MP4VideoRecorder::EncodeAndWriteFrame, this);

    return true;
}

bool MP4VideoRecorder::WriteFrame(AVFrame* frame) {
    int ret;
    ret = avcodec_send_frame(encoder_ctx_, frame);
    if (ret < 0) {
        log_error("Error sending a frame to the encoder: %s", poca_err2str(ret));
        return false;
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(encoder_ctx_, dst_video_pkt_);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        else if (ret < 0) {
            log_error("Error encoding a frame: %s", poca_err2str(ret));
            return false;
        }

        /* rescale output packet timestamp values from codec to stream timebase */
        av_packet_rescale_ts(dst_video_pkt_, encoder_ctx_->time_base, dst_video_stream_->time_base);
        dst_video_pkt_->stream_index = dst_video_stream_->index;

        AVRational* time_base = &dst_fmt_ctx_->streams[dst_video_pkt_->stream_index]->time_base;
        log_debug("pts:%s pts_time:%s dts:%s dts_time:%s stream_index:%d", poca_ts2str(dst_video_pkt_->pts).c_str(),
                  poca_ts2timestr(dst_video_pkt_->pts, time_base).c_str(), poca_ts2str(dst_video_pkt_->dts).c_str(),
                  poca_ts2timestr(dst_video_pkt_->dts, time_base).c_str(), dst_video_pkt_->stream_index);

        ret = av_interleaved_write_frame(dst_fmt_ctx_, dst_video_pkt_);
        if (ret < 0) {
            log_error("Error while writing output packet: %s", poca_err2str(ret));
            return false;
        }
    }
    return true;
}

void MP4VideoRecorder::EncodeAndWriteFrame() {
    int64_t next_pts = 0;
    while (true) {
        AVFrame* frame = ring_fifo_av_frame_full_->Get();
        if (frame == nullptr) {
            log_info("Stop send frame");
            break;
        }
        frame->pts = next_pts++;
        if (!WriteFrame(frame)) {
            log_warn("Something wrong when writing a frame");
            break;
        }

        ring_fifo_av_frame_empty_->Put(frame);
    }
    if (!WriteFrame(nullptr)) {
        log_warn("Something wrong when flushing");
    }
    av_write_trailer(dst_fmt_ctx_);
}

bool MP4VideoRecorder::SendVideoFrame(void* data, int size) {
    if (size != width_ * height_ * 3) {
        log_warn("Video frame data size not match, need: %d, actual: %d", width_ * height_ * 3, size);
    }

    AVFrame* frame;
    if (!ring_fifo_av_frame_empty_->GetNoWait(frame)) {
        log_warn("Buffer full, please put frame slowly");
        return false;
    }

    libyuv::RAWToI420((const uint8_t*)data, width_ * 3, frame->data[0], width_, frame->data[1], width_ >> 1,
                      frame->data[2], width_ >> 1, width_, height_);

    ring_fifo_av_frame_full_->PutNoWait(frame);

    return true;
}
bool MP4VideoRecorder::SendVideoFrameBlock(void* data, int size) {
    if (size != width_ * height_ * 3) {
        log_warn("Video frame data size not match, need: %d, actual: %d", width_ * height_ * 3, size);
    }

    AVFrame* frame = ring_fifo_av_frame_empty_->Get();
    libyuv::RAWToI420((const uint8_t*)data, width_ * 3, frame->data[0], width_, frame->data[1], width_ >> 1,
                      frame->data[2], width_ >> 1, width_, height_);

    ring_fifo_av_frame_full_->Put(frame);
    return true;
}

bool MP4VideoRecorder::SendAudioFrame(void* data, int size) { return false; }

bool MP4VideoRecorder::SendAudioFrameBlock(void* data, int size) { return false; }

bool MP4VideoRecorder::Stop() {
    ring_fifo_av_frame_full_->Put(nullptr);
    worker_thread_.join();

    avcodec_free_context(&encoder_ctx_);
    AVFrame* frame;
    while (ring_fifo_av_frame_full_->GetNoWait(frame)) {
        av_frame_free(&frame);
    }
    while (ring_fifo_av_frame_empty_->GetNoWait(frame)) {
        av_frame_free(&frame);
    }
    av_packet_free(&dst_video_pkt_);

    if (!(dst_fmt_ctx_->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&dst_fmt_ctx_->pb);
    }

    avformat_free_context(dst_fmt_ctx_);

    delete ring_fifo_av_frame_empty_;
    delete ring_fifo_av_frame_full_;
    return true;
}