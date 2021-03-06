//
// Created by 24909 on 2020/7/20.
//

#ifndef AUDIOVIDEOTRANSAPP_EGLDISPLAYYUV_H
#define AUDIOVIDEOTRANSAPP_EGLDISPLAYYUV_H

#include "LogUtils.h"
#include "GlobalContexts.h"
#include <EGL/egl.h>

class EGLDisplayYUV {
public:
    EGLDisplayYUV(ANativeWindow * window, GlobalContexts *context);
    ~EGLDisplayYUV();
    int eglOpen();
    int eglClose();
    ANativeWindow * nativeWindow;
    GlobalContexts *global_context;
};


#endif //AUDIOVIDEOTRANSAPP_EGLDISPLAYYUV_H
