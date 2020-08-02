//
// Created by 24909 on 2020/7/22.
//

#include <libyuv/rotate.h>
#include "VideoDataEncoder.h"

void * task_video_encode(void * pVoid) {
    VideoDataEncoder * videoDataEncoder = static_cast<VideoDataEncoder *>(pVoid);
    videoDataEncoder->encoderVideoData();
    return 0;
}

void task_video_data_release(u_char **data) {
    delete *data;
    *data = NULL;
    return;
}

VideoDataEncoder::VideoDataEncoder(int video_width, int video_height) {
    this->video_width = video_width;
    this->video_height = video_height;
    this->current_time = 0;
    this->is_encode_working = false;
    this->yuv_data_queue.setReleaseCallback(task_video_data_release);
    initVideoEncoder();
    openVideoEncoder();
    startEncoderThread();
}

VideoDataEncoder::~VideoDataEncoder() {
}

//当yuv_data == null时，停止编码
void VideoDataEncoder::addVideoData(u_char *yuv_data) {
    if(yuv_data != NULL) {
        this->yuv_data_queue.push(yuv_data);
    } else {
        LOGD("stopEncoderThread queue size : %d", this->yuv_data_queue.queueSize());
        stopEncoderThread();
    }
}

//不能进行实时编码，否则画面卡顿，或者新建线程
//相机数据会有一定旋转角度，需要注意
void VideoDataEncoder::encoderVideoData() {
    LOGD("encoderVideoData in");
    ret = av_frame_make_writable(frame);
    if(ret < 0) {
        av_strerror(ret, errors, 1024);
        LOGD("av_frame_make_writable fail, ret: %d errors : %s", ret, errors);
    }

    u_char *src_yuv_data = NULL;

    // 待没有新数据加入且原有队列数据编码完成
    while (this->is_encode_working || (!yuv_data_queue.isEmpty())) {
        frame->linesize[0] = this->video_width;
        frame->linesize[1] = this->video_width / 2;
        frame->linesize[2] = this->video_width / 2;
        yuv_data_queue.pop(src_yuv_data);

        frame->data[0] = src_yuv_data;
        frame->data[1] = src_yuv_data + this->video_width * this->video_height;
        frame->data[2] = src_yuv_data + + this->video_width * this->video_height
                + this->video_width * this->video_height / 4;

        frame->pts = this->current_time;
        encodeVideo(codec_cxt, frame, pkt, dst_file);
        this->current_time += 1;
        task_video_data_release(&src_yuv_data);
    }

    closeVideoEncoder();
    LOGD("encoderVideoData out");
}

void VideoDataEncoder::openVideoEncoder() {
    codec_cxt = avcodec_alloc_context3(codec);
    if (!codec_cxt) {
        LOGD("codec_cxt alloc fail");
        return;
    }

    //设置codec 参数
    codec_cxt->bit_rate = 1820000;
    //bit_rate: bit每秒
    // 参考：https://docs.agora.io/cn/Video/video_profile_android?platform=Android

    codec_cxt->width = this->video_width;
    codec_cxt->height = this->video_height;

    //帧率  每秒25帧, Camera采样设置
    codec_cxt->time_base = (AVRational){1, 25};
    codec_cxt->framerate = (AVRational){25, 1};
    codec_cxt->gop_size = 10;
    codec_cxt->max_b_frames = 1;
    codec_cxt->pix_fmt = AV_PIX_FMT_YUV420P;

    //slow, 保证视频压缩质量
    av_opt_set(codec_cxt->priv_data, "preset", "slow", 0);

    //打开编码器
    int ret = avcodec_open2(codec_cxt, codec, NULL);
    if (ret < 0) {
        av_strerror(ret, errors, 1024);
        LOGD("avcodec_open2 fail ret : %d error: %s", ret, errors);
        return;
    }

    frame = av_frame_alloc();
    if(NULL == frame) {
        LOGD("frame alloc fail");
        return;
    }

    pkt = av_packet_alloc();
    if(NULL == pkt) {
        LOGD("pkt alloc fail");
        return;
    }

    frame->format = codec_cxt->pix_fmt;
    frame->width  = codec_cxt->width;
    frame->height = codec_cxt->height;

    ret = av_frame_get_buffer(frame, 32);
    if(ret <  0) {
        LOGD("av_frame_get_buffer fail");
        return;
    }
}

void VideoDataEncoder::initVideoEncoder() {
    dst_file = fopen(dst_file_name, "wb");
    if(NULL == dst_file) {
        LOGD("file open fail : %s", dst_file_name);
        return;
    }

    codec = avcodec_find_encoder_by_name(codec_name);
    if (!codec) {
        LOGD("codec find fail : %s", codec_name);
        return;
    } else {
        LOGD("codec find success : %s", codec_name);
    }
}

void VideoDataEncoder::closeVideoEncoder() {
    LOGD("closeVideoEncoder in");
    encodeVideo(codec_cxt, NULL, pkt, dst_file);
    fwrite(endcode, 1, sizeof(endcode), dst_file);

    if(NULL != dst_file) {
        fflush(dst_file);
        fclose(dst_file);
    }

    if(codec_cxt != NULL) {
        avcodec_close(codec_cxt);
        avcodec_free_context(&codec_cxt);
    }

    if(NULL != frame) {
        av_frame_free(&frame);
        frame = NULL;
    }

    if(NULL != pkt) {
        av_packet_free(&pkt);
        pkt = NULL;
    }
    LOGD("closeVideoEncoder out");
}

void VideoDataEncoder::encodeVideo(AVCodecContext *pContext, AVFrame *pFrame, AVPacket *pPacket,
                                   FILE *pFile) {
    LOGD("encodeVideo in");
    int ret;
    /* send the frame to the encoder */
    ret = avcodec_send_frame(pContext, pFrame);
    if (ret < 0) {
        LOGD("avcodec_send_frame fail");
        return;
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(pContext, pPacket);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            av_strerror(ret, errors, 1024);
            LOGD("avcodec_receive_packet : %d  : %s ", ret, errors);
            return;
        } else if (ret < 0) {
            av_strerror(ret, errors, 1024);
            LOGD("avcodec_receive_packet : %d  : %s ", ret, errors);
            break;
        }
        fwrite(pPacket->data, 1, pPacket->size, pFile);
        av_packet_unref(pPacket);
    }
    LOGD("encodeVideo out");
}

void VideoDataEncoder::startEncoderThread() {
    LOGD("startEncoderThread in");
    yuv_data_queue.setFlag(1);
    this->is_encode_working = true;
    // 1.新建编码线程
    pthread_create(&pid_video_encode, 0, task_video_encode, this);
    LOGD("startEncoderThread out");
}

void VideoDataEncoder::stopEncoderThread() {
    LOGD("stopEncoderThread in");
    yuv_data_queue.setFlag(0);
    this->is_encode_working = false;
    LOGD("stopEncoderThread out");
}
