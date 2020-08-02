# AudioVideoTransApp
基于Android平台音视频软编码的简单教程，串联整个音视频编码流程，完成音频采集，音频编码，视频采集，视频渲染，视频编码等等

#### 编译环境
FFmpeg版本： **4.2.2**    
NDK版本：**r17c** 
#### 运行环境
* x86(模拟器)
* arm64-v8a(64位手机)
#### 功能点
* 使用AudioRecord音频采集
* 使用Camera视频采集
* 使用OpenGL视频渲染
* 使用FFmpeg音视频编码，输出音频压缩文件(aac)，视频压缩文件(h264)
* 集成libfdkaac,h264,libyuv，FFmpeg等开源库
#### 运行效果(工程根目录images文件夹)
![渲染效果](https://github.com/wanglongsoft/AudioVideoTransApp/tree/master/images/display.png)
