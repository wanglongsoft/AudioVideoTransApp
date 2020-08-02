//
// Created by 24909 on 2020/7/22.
//

#ifndef AUDIOVIDEOTRANSAPP_AUDIODATAENCODER_H
#define AUDIOVIDEOTRANSAPP_AUDIODATAENCODER_H


#include <jni.h>
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

//添加线程，设置队列
class AudioDataEncoder {
public:
    AudioDataEncoder();
    ~AudioDataEncoder();
    void addAudioData(jbyteArray data, int length, JNIEnv *env);
    void readAudioDataFromPcmFile();
    void encoderAudioData();
private:

    void openAudioEncoder();
    void initAudioEncoder();
    void closeAudioEncoder();
    void startEncoderThread();
    void stopEncoderThread();
    void readAudioDataFromByteArray(uint16_t *pInt, jbyteArray pArray, int length, JNIEnv *env,
            int index, int frame_size);
    void encodeAudio(AVCodecContext *pContext, AVFrame *pFrame, AVPacket *pPacket, FILE *pFile);
    void readAudioFrameFromPCMFile(uint16_t *data, FILE *pFile, uint64_t index, int data_size);

    FILE *dst_fd = NULL;
    FILE *src_fd = NULL;
    AVFrame *frame = NULL;
    AVPacket *pkt = NULL;
    uint16_t *samples = NULL;

    const char *codec_name = "libfdk_aac";
    const char *src_file_name = "/storage/emulated/0/filefilm/audio_capture.pcm";
    const char *dst_file_name = "/storage/emulated/0/filefilm/audio_capture.aac";
    AVCodec *codec = NULL;
    AVCodecContext *context = NULL;
    bool is_audio_encoder_open;
    int audio_frame_size;

    SafeQueue<uint16_t *> pcm_data_queue;
    pthread_t pid_audio_encode;
    bool is_encode_working;
};


#endif //AUDIOVIDEOTRANSAPP_AUDIODATAENCODER_H
