#include <jni.h>
#include <string>

#include "LogUtils.h"
#include "GlobalContexts.h"
#include "EGLDisplayYUV.h"
#include "ShaderYUV.h"
#include "AudioDataEncoder.h"
#include "VideoDataEncoder.h"

#include <libyuv/convert.h>

#include <pthread.h>
#include <android/native_window_jni.h>
#include <android/asset_manager_jni.h>
#include <android/asset_manager.h>

ANativeWindow * nativeWindow = NULL;
GlobalContexts *global_context = NULL;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
EGLDisplayYUV *eglDisplayYuv = NULL;
ShaderYUV * shaderYuv = NULL;
AudioDataEncoder* audioDataEncoder = NULL;
VideoDataEncoder* videoDataEncoder = NULL;

//unsigned char* array_data = NULL;
unsigned char* yuv_array_data = NULL;
unsigned char* yuv_array_rotate_data = NULL;
unsigned char* yuv_array_mirror_data = NULL;

unsigned char* convertJByteaArrayToChars(JNIEnv *env, jbyteArray bytearray);
unsigned char* convertNV21ToYUV420P(unsigned char* array_data, int width, int height);

extern "C"
JNIEXPORT void JNICALL
Java_com_soft_function_FunctionControl_setSurface(JNIEnv *env, jobject thiz, jobject surface) {
    LOGD("setSurface in");
    pthread_mutex_lock(&mutex);
    if (nativeWindow) {
        LOGD("setSurface nativeWindow != NULL");
        ANativeWindow_release(nativeWindow);
        nativeWindow = NULL;
    }

    // 创建新的窗口用于视频显示
    nativeWindow = ANativeWindow_fromSurface(env, surface);
    if(NULL == global_context) {
        LOGD("new GlobalContext");
        global_context = new GlobalContexts();
    }
    global_context->nativeWindow = nativeWindow;

    if(NULL != eglDisplayYuv) {
        LOGD("eglDisplayYuv->eglClose");
        eglDisplayYuv->eglClose();
        delete eglDisplayYuv;
        eglDisplayYuv = NULL;
    }
    if(NULL != shaderYuv) {
        delete shaderYuv;
        shaderYuv = NULL;
    }
    pthread_mutex_unlock(&mutex);
    LOGD("setSurface out");
}

extern "C"
JNIEXPORT void JNICALL
Java_com_soft_function_FunctionControl_setSurfaceSize(JNIEnv *env, jobject thiz, jint width,
                                                      jint height) {
    LOGD("setSurfaceSize in");
    pthread_mutex_lock(&mutex);
    if(NULL == global_context) {
        global_context = new GlobalContexts();
        global_context->gl_video_width = height;
        global_context->gl_video_height = width;
    }
    global_context->gl_window_width = width;
    global_context->gl_window_height = height;
    pthread_mutex_unlock(&mutex);
    LOGD("setSurfaceSize out");
}

extern "C"
JNIEXPORT void JNICALL
Java_com_soft_function_FunctionControl_saveAssetManager(JNIEnv *env, jobject thiz,
                                                        jobject manager) {
    LOGD("saveAssetManager in");
    pthread_mutex_lock(&mutex);
    AAssetManager *mgr = AAssetManager_fromJava(env, manager);
    if(NULL == global_context) {
        global_context = new GlobalContexts();
    }
    global_context->assetManager = mgr;
    pthread_mutex_unlock(&mutex);
    LOGD("saveAssetManager out");
}

extern "C"
JNIEXPORT void JNICALL
Java_com_soft_function_FunctionControl_rendSurface(JNIEnv *env, jobject thiz, jbyteArray data,
                                                   jint width, jint height, jint video_rotation) {
    LOGD("rendSurface in video_rotation: %d", video_rotation);

    if(NULL != yuv_array_rotate_data) {
        delete yuv_array_rotate_data;
        yuv_array_rotate_data = NULL;
    }

    if(NULL != yuv_array_mirror_data) {
        delete yuv_array_mirror_data;
        yuv_array_mirror_data = NULL;
    }

    if(NULL == videoDataEncoder) {
        //手机摄像头，需将数据旋转90/270，因此需要调整宽高
        videoDataEncoder = new VideoDataEncoder(height, width);
    }

    if(NULL == data || video_rotation == -1) {
        //libx264 结束视频编码
        videoDataEncoder->addVideoData(NULL);
        return;
    }

    jbyte *src_data = env->GetByteArrayElements(data, NULL);

    int src_y_size = width * height;
    int src_u_size = (width >> 1) * (height >> 1);

    yuv_array_data = new uint8[width * height * 3 / 2];
    yuv_array_rotate_data = new uint8[width * height * 3 / 2];

    jbyte *src_nv21_y_data = src_data;
    jbyte *src_nv21_vu_data = src_data + src_y_size;

    uint8_t *dst_y_rotate_data = yuv_array_rotate_data;
    uint8_t *dst_u_rotate_data = yuv_array_rotate_data + src_y_size;
    uint8_t *dst_v_rotate_data = yuv_array_rotate_data + src_y_size + src_u_size;

    libyuv::NV21ToI420((const uint8 *) src_nv21_y_data, width,
                       (const uint8 *) src_nv21_vu_data, width,
                       dst_y_rotate_data, width,
                       dst_u_rotate_data, width >> 1,
                       dst_v_rotate_data, width >> 1,
                       width, height);

    uint8_t *dst_y_data = yuv_array_data;
    uint8_t *dst_u_data = yuv_array_data + src_y_size;
    uint8_t *dst_v_data = yuv_array_data + src_y_size + src_u_size;

    if(video_rotation == 270)  {//前置摄像头
        yuv_array_mirror_data = new uint8[width * height * 3 / 2];
        uint8_t *mirror_y_data = yuv_array_mirror_data;
        uint8_t *mirror_u_data = yuv_array_mirror_data + src_y_size;
        uint8_t *mirror_v_data = yuv_array_mirror_data + src_y_size + src_u_size;
        libyuv::I420Rotate(
                dst_y_rotate_data, width,
                dst_u_rotate_data, width >> 1,
                dst_v_rotate_data, width >> 1,
                mirror_y_data, height,
                mirror_u_data, height >> 1,
                mirror_v_data, height >> 1,
                width, height,
                libyuv::kRotate270);
        libyuv::I420Mirror(//左右镜像
                mirror_y_data, height,
                mirror_u_data, height >> 1,
                mirror_v_data, height >> 1,
                dst_y_data, height,
                dst_u_data, height >> 1,
                dst_v_data, height >> 1,
                height, width);

    } else {//后置摄像头，video_rotation == 90
        libyuv::I420Rotate(
                dst_y_rotate_data, width,
                dst_u_rotate_data, width >> 1,
                dst_v_rotate_data, width >> 1,
                dst_y_data, height,
                dst_u_data, height >> 1,
                dst_v_data, height >> 1,
                width, height,
                libyuv::kRotate90);
    }

    unsigned char* frame_data[3];

    frame_data[0] = dst_y_data;
    frame_data[1] = dst_u_data;
    frame_data[2] = dst_v_data;

    //渲染
    if(NULL == global_context) {
        global_context = new GlobalContexts();
    }

    if(NULL == eglDisplayYuv) {
        global_context->gl_video_width = width;
        global_context->gl_video_height = height;
        eglDisplayYuv = new EGLDisplayYUV(global_context->nativeWindow, global_context);
        eglDisplayYuv->eglOpen();
    }

    if(NULL == shaderYuv) {
        //图像旋转90或者270，需要调整宽高
        global_context->gl_video_width = height;
        global_context->gl_video_height = width;
        shaderYuv = new ShaderYUV(global_context);
        shaderYuv->CreateProgram();
    }
    shaderYuv->Render(frame_data);

    //libx264 进行视频编码
    videoDataEncoder->addVideoData(yuv_array_data);

    pthread_mutex_unlock(&mutex);
    LOGD("rendSurface out");
}

extern "C"
JNIEXPORT void JNICALL
Java_com_soft_function_FunctionControl_rendAudio(JNIEnv *env, jobject thiz, jbyteArray data,
                                                 jint length) {
    LOGD("rendAudio in");
    if(NULL == audioDataEncoder) {
        audioDataEncoder = new AudioDataEncoder();
    }
    audioDataEncoder->addAudioData(data, length, env);
    LOGD("rendAudio out");
}

extern "C"
JNIEXPORT void JNICALL
Java_com_soft_function_FunctionControl_rendAudioFromPcmFile(JNIEnv *env, jobject thiz) {
    // TODO: implement rendAudioFromPcmFile()
    LOGD("rendAudioFromPcmFile in");
    if(NULL == audioDataEncoder) {
        audioDataEncoder = new AudioDataEncoder();
    }
    audioDataEncoder->readAudioDataFromPcmFile();
    LOGD("rendAudioFromPcmFile out");
}

extern "C"
JNIEXPORT void JNICALL
Java_com_soft_function_FunctionControl_releaseResources(JNIEnv *env, jobject thiz) {
    LOGD("releaseResources in");
    if(NULL != global_context) {
        delete global_context;
        global_context = NULL;
    }
    if(NULL != eglDisplayYuv) {
        delete eglDisplayYuv;
        eglDisplayYuv = NULL;
    }
    if(NULL != shaderYuv) {
        delete shaderYuv;
        shaderYuv = NULL;
    }

//    if(NULL != array_data) {
//        delete array_data;
//        array_data = NULL;
//    }

    //编码完成，由编码器类释放内存
//    if(NULL != yuv_array_data) {
//        delete yuv_array_data;
//        yuv_array_data = NULL;
//    }
    if(NULL != yuv_array_data) {
        yuv_array_data = NULL;
    }

    if(NULL != yuv_array_rotate_data) {
        delete yuv_array_rotate_data;
        yuv_array_rotate_data = NULL;
    }


    if(NULL != yuv_array_mirror_data) {
        delete yuv_array_mirror_data;
        yuv_array_mirror_data = NULL;
    }

    if(NULL != audioDataEncoder) {
        delete audioDataEncoder;
        audioDataEncoder = NULL;
    }

    if(NULL != videoDataEncoder) {
        delete videoDataEncoder;
        videoDataEncoder = NULL;
    }

    LOGD("releaseResources out");
}

unsigned char* convertJByteaArrayToChars(JNIEnv *env, jbyteArray bytearray)
{
    unsigned char *chars = NULL;
    jbyte *bytes;
    bytes = env->GetByteArrayElements(bytearray, 0);
    int chars_len = env->GetArrayLength(bytearray);
    chars = new unsigned char[chars_len];//使用结束后, delete 该数组
    memset(chars,0,chars_len);
    memcpy(chars, bytes, chars_len);
    chars[chars_len] = 0;
    env->ReleaseByteArrayElements(bytearray, bytes, 0);
    return chars;
}

unsigned char* convertNV21ToYUV420P(unsigned char* array_data, int width, int height) {
    unsigned char *src_chars = array_data;
    unsigned char *dst_chars = NULL;
    dst_chars = new unsigned char[width * height / 2 * 3];
    int uv_data_size = width * height / 4;
    int y_data_size = width * height;
    for (int i = 0; i < y_data_size; ++i) {
        dst_chars[i] = src_chars[i];
    }
    for (int i = 0; i < uv_data_size; ++i) {
        dst_chars[i + y_data_size] = src_chars[2 * i + 1 + y_data_size];//  U数据存储
        dst_chars[i + y_data_size + uv_data_size] = src_chars[2 * i + y_data_size];//  V数据存储
    }
    return dst_chars;
}
