/*! \file ConnectToAgora.h */
#pragma once

#include <csignal>
#include <stdio.h>
#include <cstring>
#include <sstream>
#include <string>
#include <thread>

#include "IAgoraService.h"
#include "NGIAgoraRtcConnection.h"

#include "common/helper.h"
#include "common/opt_parser.h"
#include "common/sample_common.h"
#include "common/sample_connection_observer.h"
/*#include "utils/log.h"
*/


//设置声网传输音视频参数
#define DEFAULT_CONNECT_TIMEOUT_MS (3000)
#define DEFAULT_SAMPLE_RATE (48000)
#define DEFAULT_NUM_OF_CHANNELS (2)
#define DEFAULT_TARGET_BITRATE (1 * 1000 * 1000)
#define DEFAULT_VIDEO_WIDTH (1920)
#define DEFAULT_VIDEO_HEIGHT (1080)
#define DEFAULT_FRAME_RATE (25)
#define DEFAULT_AUDIO_FILE "test_data/audio.raw"
#define DEFAULT_VIDEO_FILE "test_data/vieo.raw"

/**
 * @brief
 * 主要用来配置需要连接的token、channel和user的id，以及发送的video以及audio的基本参数
 */
struct SampleOptions {
  std::string appId;
  std::string channelId;
  std::string userId;
  std::string audioFile = DEFAULT_AUDIO_FILE;
  std::string videoFile = DEFAULT_VIDEO_FILE;

  struct {
    int sampleRate = DEFAULT_SAMPLE_RATE;
    int numOfChannels = DEFAULT_NUM_OF_CHANNELS;
  } audio;
  struct {
    int targetBitrate = DEFAULT_TARGET_BITRATE;
    int width = DEFAULT_VIDEO_WIDTH;
    int height = DEFAULT_VIDEO_HEIGHT;
    int frameRate = DEFAULT_FRAME_RATE;
  } video;
};

/*!
    用于连接至声网服务器，配置token以及channel等参数，并创建yuvsender用于发送yuv

    \param 无

    \return 错误码，1表示成功，其它表示失败

    \todo
*/
int connectAgora();

/*!
    用于断开声网服务器，并回收所有临时申请的内存

    \param 无

    \return 错误码，1表示成功，其它表示失败

    \todo
*/
int disconnectAgora();

/*!
    用于发送将单帧yuv数据发送至声网服务器的指定token和channel下,blackmagic每采集一帧数据便会调用该函数

    \param frameBuf 指向需要发送的单帧yuv数据的指针

    \return 错误码，1表示成功，其它表示失败

    \todo
*/
int sendOneYuvFrame(void* frameBuf);

int sendOnePcmFrame(void* frameBuf);
