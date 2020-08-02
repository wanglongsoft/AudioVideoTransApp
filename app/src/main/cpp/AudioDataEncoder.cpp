//
// Created by 24909 on 2020/7/22.
//

#include "AudioDataEncoder.h"

void * task_audio_encode(void * pVoid) {
    AudioDataEncoder * audioDataEncoder = static_cast<AudioDataEncoder *>(pVoid);
    audioDataEncoder->encoderAudioData();
    return 0;
}

void task_audio_data_release(uint16_t **data) {
    delete *data;
    *data = NULL;
    return;
}

AudioDataEncoder::AudioDataEncoder() {
    is_audio_encoder_open = false;
    initAudioEncoder();
    openAudioEncoder();
}

AudioDataEncoder::~AudioDataEncoder() {

}

void AudioDataEncoder::openAudioEncoder() {
    if(codec == NULL) {
        LOGD("openAudioEncoder fail : codec == null");
        return;
    }

    context = avcodec_alloc_context3(codec);
    if(NULL == context) {
        LOGD("avcodec_alloc_context3 context fail");
        return;
    }

    //与采集音频格式保持一致
    context->sample_fmt = AV_SAMPLE_FMT_S16;
    context->sample_rate = 44100;
    context->channel_layout = AV_CH_LAYOUT_STEREO;
    context->channels = 2;
    context->bit_rate = 176400;

    if (avcodec_open2(context, codec, NULL) < 0) {
        LOGD("avcodec_open2 fail");
        return;
    } else {
        LOGD("avcodec_open2 success");
    }

    is_audio_encoder_open = true;

    frame = av_frame_alloc();
    if(NULL == frame) {
        LOGD("av_frame_alloc frame fail");
        return;
    }

    pkt = av_packet_alloc();
    if(NULL == pkt) {
        LOGD("av_packet_alloc pkt fail");
        return;
    }

    frame->nb_samples     = context->frame_size;
    frame->format         = context->sample_fmt;
    frame->channel_layout = context->channel_layout;

    int ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        LOGD("av_frame_get_buffer fail");
        return;
    }

    //双声道 * 2，每个样本占 2 字节
    //每个音频帧数据所占的字节数
    audio_frame_size = context->frame_size * 2 * 2;

    LOGD("frame->nb_samples : %d", frame->nb_samples);
    LOGD("frame->format : %d", frame->format);
    LOGD("frame->channel_layout: %d", frame->channel_layout);
    LOGD("audio_frame_size: %d", audio_frame_size);

    startEncoderThread();
}

void AudioDataEncoder::initAudioEncoder() {
    dst_fd = fopen(dst_file_name, "wb");
    if(NULL == dst_fd) {
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

void AudioDataEncoder::closeAudioEncoder() {
    LOGD("closeAudioEncoder in");
    if(codec == NULL) {
        LOGD("closeAudioEncoder fail : codec == null");
        return;
    }

    if(!is_audio_encoder_open) {
        LOGD("closeAudioEncoder fail : codec is not open");
        return;
    }

    /* flush the encoder */
    encodeAudio(context, NULL, pkt, dst_fd);

    if(NULL != src_fd) {
        fclose(src_fd);
        src_fd = NULL;
    }

    if(NULL != dst_fd) {
        fflush(dst_fd);
        fclose(dst_fd);
        dst_fd = NULL;
    }

    is_audio_encoder_open = false;

    if(context != NULL) {
        avcodec_close(context);
        avcodec_free_context(&context);
        context = NULL;
    }
    if(frame != NULL) {
        av_frame_free(&frame);
        frame = NULL;
    }
    if(pkt != NULL) {
        av_packet_free(&pkt);
        pkt = NULL;
    }
    LOGD("closeAudioEncoder out");
}

//jbyteArray data, int length, JNIEnv *env
void AudioDataEncoder::encoderAudioData() {
    LOGD("encoderAudioData in");
    if(av_frame_make_writable(frame) < 0) {
        LOGD("av_frame_make_writable fail");
        return;
    }

    uint16_t *queue_samples = NULL;

    while (this->is_encode_working || (!pcm_data_queue.isEmpty())) {
        pcm_data_queue.pop(queue_samples);
        memcpy((uint16_t*)frame->data[0], queue_samples, audio_frame_size);
        encodeAudio(context, frame, pkt, dst_fd);
        task_audio_data_release(&queue_samples);
    }
    closeAudioEncoder();
    LOGD("encoderAudioData out");
}

void AudioDataEncoder::addAudioData(jbyteArray data, int length, JNIEnv *env) {
    if(data != NULL) {
        //每次采样数据2字节
        uint64_t audio_frame_count = ceil(((double) length) / audio_frame_size);
        for (uint64_t i = 0; i < audio_frame_count; ++i) {
            samples = new uint16_t[audio_frame_size];
            readAudioDataFromByteArray(samples, data, length, env, i, audio_frame_size);
            this->pcm_data_queue.push(samples);
        }
    } else {
        LOGD("stopEncoderThread queue size : %d", this->pcm_data_queue.queueSize());
        stopEncoderThread();
    }
}

void AudioDataEncoder::readAudioDataFromByteArray(uint16_t *pInt, jbyteArray pArray, int length,
        JNIEnv *env, int index, int frame_size) {
    jbyte *bytes;
    bytes = env->GetByteArrayElements(pArray, 0);
    int chars_len = env->GetArrayLength(pArray);
    int copy_index = frame_size * index;
    int copy_length = (chars_len - frame_size * (index + 1)) > 0 ? frame_size : (chars_len - copy_index);
    memcpy(pInt, bytes + copy_index, copy_length);
    env->ReleaseByteArrayElements(pArray, bytes, 0);
}

void AudioDataEncoder::readAudioDataFromPcmFile() {
    LOGD("readAudioDataFromPcmFile in");
    if(!is_audio_encoder_open) {
        LOGD("readAudioDataFromPcmFile fail : codec is not open");
        return;
    }

    src_fd = fopen(src_file_name, "rb");
    if(NULL == src_fd) {
        LOGD("readAudioDataFromPcmFile file open fail : %s", src_file_name);
        return;
    }

    fseek(src_fd, 0, SEEK_END);
    uint64_t file_size = ftell(src_fd);
    int totalSeconds = file_size / (context->sample_rate * context->channels * 2);
    LOGD("readAudioDataFromPcmFile totalSeconds : %d", totalSeconds);

    int audio_frame_size = context->frame_size * 2;//双通道采样次数
    uint64_t audio_frame_count = ceil(((double) file_size) / audio_frame_size / 2);//每次采样数据2字节

    LOGD("readAudioDataFromPcmFile audio_frame_size : %d", audio_frame_size);
    LOGD("readAudioDataFromPcmFile audio_frame_count : %d", audio_frame_count);

    int ret = 0;
    for (uint64_t i = 0; i < audio_frame_count; ++i) {
        ret = av_frame_make_writable(frame);
        if(ret < 0) {
            LOGD("av_frame_make_writable fail");
            return;
        }
        samples = (uint16_t*)frame->data[0];
        if(i == (audio_frame_count - 1)) {//防止文件读取越界
            audio_frame_size = (file_size - i * context->frame_size * 2 * 2);
        }
        readAudioFrameFromPCMFile(samples, src_fd, i, audio_frame_size);
        encodeAudio(context, frame, pkt, dst_fd);
    }

    closeAudioEncoder();
    LOGD("readAudioDataFromPcmFile out");
}

void AudioDataEncoder::encodeAudio(AVCodecContext *pContext, AVFrame *pFrame, AVPacket *pPacket,
                                   FILE *pFile) {
    int ret;

    /* send the frame for encoding */
    ret = avcodec_send_frame(pContext, pFrame);
    if (ret < 0) {
        LOGD("avcodec_send_frame ret : %d", ret);
        return;
    }

    /* read all the available output packets (in general there may be any
     * number of them */
    while (ret >= 0) {
        ret = avcodec_receive_packet(pContext, pPacket);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            LOGD("avcodec_receive_packet fail : %d", ret);
            return;
        }

        fwrite(pPacket->data, 1, pPacket->size, pFile);
        av_packet_unref(pPacket);
    }
}

void AudioDataEncoder::readAudioFrameFromPCMFile(uint16_t *data, FILE *pFile,
        uint64_t index, int data_size) {
    LOGD("readAudioFrameFromPCMFile in data_size : %d", data_size);
    int result;
    uint64_t data_offset;
    FILE *audio_data_fd = pFile;
    data_offset = index * 1024 * 2 * 2;
    //seek到偏移处
    result = fseek(audio_data_fd, data_offset, SEEK_SET);
    if (result == -1)
    {
        LOGD("readAudioFrameFromPCMFile fail, fseek fail");
        return;
    }

    fread(data, 2, data_size, audio_data_fd);

    LOGD("readAudioFrameFromPCMFile out");
}

void AudioDataEncoder::startEncoderThread() {
    LOGD("startEncoderThread in");
    pcm_data_queue.setFlag(1);
    this->is_encode_working = true;
    // 1.新建编码线程
    pthread_create(&pid_audio_encode, 0, task_audio_encode, this);
    LOGD("startEncoderThread out");
}

void AudioDataEncoder::stopEncoderThread() {
    LOGD("stopEncoderThread in");
    pcm_data_queue.setFlag(0);
    this->is_encode_working = false;
    LOGD("stopEncoderThread out");
}

