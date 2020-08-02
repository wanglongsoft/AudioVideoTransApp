//
// Created by 24909 on 2020/7/20.
//

#ifndef AUDIOVIDEOTRANSAPP_SHADERUTILS_H
#define AUDIOVIDEOTRANSAPP_SHADERUTILS_H

#include "LogUtils.h"
#include <cstring>
#include <string>
#include <android/asset_manager.h>

class ShaderUtils {
public:
    static std::string * openAssetsFile(AAssetManager *mgr, char *file_name);
};


#endif //AUDIOVIDEOTRANSAPP_SHADERUTILS_H
