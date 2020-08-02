//
// Created by 24909 on 2020/7/22.
//

#ifndef VideoVIDEOTRANSAPP_VIDEODATAENCODER_H
#define VideoVIDEOTRANSAPP_VIDEODATAENCODER_H

#include <cstdio>
#include "LogUtils.h"
#include "SafeQueue.h"

extern "C" {
    #include "ffmpeg/include/libavformat/avformat.h"
    #include "ffmpeg/include/libavutil/avutil.h"
    #include "ffmpeg/include/libavformat/avformat.h"
    #include "ffmpeg/include/libavutil/intreadwrite.h"
    #include "ffmpeg/include/libavutil/timestamp.h"
    #include "ffmpeg/include/libavutil/opt.h"
    #include "ffmpeg/include/libavutil/time.h"
    #include "ffmpeg/include/libswresample/swresample.h"
    #include "ffmpeg/include/libswscale/swscale.h"
}

class VideoDataEncoder {
public:
    VideoDataEncoder(int video_width, int video_height);
    ~VideoDataEncoder();
    //处理YUV420P数据(UV数据不交叉存储)，NV12、NV21(UV数据交叉存储)
    void addVideoData(u_char *yuv_data);
    void encoderVideoData();
private:
    void openVideoEncoder();
    void initVideoEncoder();
    void closeVideoEncoder();
    void startEncoderThread();
    void stopEncoderThread();
    void encodeVideo(AVCodecContext *pContext, AVFrame *pFrame, AVPacket *pPacket, FILE *pFile);

    FILE *dst_file = NULL;
    const char *codec_name = "libx264";
    const char *dst_file_name = "/storage/emulated/0/filefilm/video_capture.h264";
    AVFrame *frame = NULL;
    AVPacket *pkt = NULL;
    AVCodec *codec = NULL;
    AVCodecContext *codec_cxt = NULL;
    char errors[1024];
    int video_width;
    int video_height;
    int ret;
    int64_t current_time;
    uint8_t endcode[4] = { 0, 0, 1, 0xb7 };
    SafeQueue<u_char *> yuv_data_queue;
    pthread_t pid_video_encode;
    bool is_encode_working;
};


#endif //VideoVIDEOTRANSAPP_VIDEODATAENCODER_H
