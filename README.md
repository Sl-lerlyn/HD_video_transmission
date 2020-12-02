HD_video_transmission是基于声网SDK以及Blackmagic开发的高清视频实时传输系统。本系统将视音频信号（HDMI接口或者SDI接口）通过输入集板卡A接入并转成YUV信号，发送到互联网上；另一台电脑B将视音频信息从网上拉下来并转成YUV信号转送给输出采集卡B并转成视频信号实时输出播放（HDMI和SDI接口同时输出）。本项目是HD_video_transmission的发送端。

1、编译环境
ubuntu 20.04
QT 5.9.4
gcc/g++

2、依赖库
Blackmagic DeckLink SDK 11.5.1
Agora_Native_SDK_for_Linux_x64_rel.v2.7.1.909_FULL_20200731_1130

3、运行
在CapturePreview.pro中设置需导入的动态库及头文件地址。
在ConnectToAgora.cpp中设置从声网注册的appId、channelId以及userId。
使用QT打开apturePreview.pro文件编译运行。