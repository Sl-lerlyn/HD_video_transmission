#include "ConnectToAgora.h"

/*static void SampleSendAudioTask(
    const SampleOptions& options,
    agora::agora_refptr<agora::rtc::IAudioPcmDataSender> audioPcmDataSender, bool& exitFlag) {
  // Currently only 10 ms PCM frame is supported. So PCM frames are sent at 10 ms interval
  PacerInfo pacer = {0, 10, std::chrono::steady_clock::now()};

  while (!exitFlag) {
    sendOnePcmFrame(options, audioPcmDataSender);
    waitBeforeNextSend(pacer);  // sleep for a while before sending next frame
  }
}

static void SampleSendVideoTask(const SampleOptions& options,
                                agora::agora_refptr<agora::rtc::IVideoFrameSender> videoFrameSender,
                                bool& exitFlag) {
  // Calculate send interval based on frame rate. H264 frames are sent at this interval
  PacerInfo pacer = {0, 1000 / options.video.frameRate, std::chrono::steady_clock::now()};

  while (!exitFlag) {
    sendOneYuvFrame(options, videoFrameSender);
    waitBeforeNextSend(pacer);  // sleep for a while before sending next frame
  }
}*/


SampleOptions options;
agora::base::IAgoraService *service;
agora::rtc::RtcConnectionConfiguration ccfg;
agora::agora_refptr<agora::rtc::IRtcConnection> connection;
std::shared_ptr<SampleConnectionObserver> connObserver;
agora::agora_refptr<agora::rtc::IMediaNodeFactory> factory;
agora::agora_refptr<agora::rtc::IVideoFrameSender> videoFrameSender;
agora::agora_refptr<agora::rtc::ILocalVideoTrack> customVideoTrack;
agora::agora_refptr<agora::rtc::IAudioPcmDataSender> audioPcmDataSender;
agora::agora_refptr<agora::rtc::ILocalAudioTrack> customAudioTrack;

int connectAgora()
{

    options.appId = "00606d5161998b4427e9476ea06b3015425IABjU/mbvziPE3s83IbQUcSZo6zVW+FguLXnces7lFq+swx+f9gAAAAAEAAT20h7PPGkXwEAAQA78aRf";
    options.channelId = "test";

    options.userId = "0";

    // Create Agora service
    service = createAndInitAgoraService(false, true, true);
    if (!service) {
      printf("Failed to creating Agora service!\n");
    }

    // Create Agora connection
    ccfg.autoSubscribeAudio = false;
    ccfg.autoSubscribeVideo = false;
    ccfg.clientRoleType = agora::rtc::CLIENT_ROLE_BROADCASTER;

    connection = service->createRtcConnection(ccfg);
    if (!connection) {
      printf("Failed to creating Agora connection!\n");
      return -1;
    }

    // Register connection observer to monitor connection event
    connObserver = std::make_shared<SampleConnectionObserver>();
    connection->registerObserver(connObserver.get());

    // Connect to Agora channel
    if (connection->connect(options.appId.c_str(), options.channelId.c_str(),
                            options.userId.c_str())) {
      printf("Failed to connect to Agora channel!\n");
      return -1;
    }

    // Create media node factory
    factory = service->createMediaNodeFactory();
    if (!factory) {
      printf("Failed to create media node factory!\n");
    }

    // Create audio data sender
    audioPcmDataSender =
        factory->createAudioPcmDataSender();
    if (!audioPcmDataSender) {
      printf("Failed to create audio data sender!\n");
      return -1;
    }

    // Create audio track
    customAudioTrack =
        service->createCustomAudioTrack(audioPcmDataSender);
    if (!customAudioTrack) {
      printf("Failed to create audio track!\n");
      return -1;
    }

    // Create video frame sender
    videoFrameSender =
        factory->createVideoFrameSender();
    if (!videoFrameSender) {
      printf("Failed to create video frame sender!\n");
      return -1;
    }

    // Create video track
    customVideoTrack = service->createCustomVideoTrack(videoFrameSender);
    if (!customVideoTrack) {
      printf("Failed to create video track!\n");
      return -1;
    }

    // Configure video encoder
    agora::rtc::VideoEncoderConfiguration encoderConfig(
        options.video.width, options.video.height, options.video.frameRate,
        options.video.targetBitrate, agora::rtc::ORIENTATION_MODE_ADAPTIVE);
    customVideoTrack->setVideoEncoderConfiguration(encoderConfig);

    // Publish audio & video track
    customAudioTrack->setEnabled(true);
    connection->getLocalUser()->publishAudio(customAudioTrack);
    customVideoTrack->setEnabled(true);
    connection->getLocalUser()->publishVideo(customVideoTrack);

    // Wait until connected before sending media stream
    connObserver->waitUntilConnected(DEFAULT_CONNECT_TIMEOUT_MS);

    // Start sending media data
    //printf("Start sending audio & video data ...\n");
    /*std::thread sendAudioThread(SampleSendAudioTask, options, audioPcmDataSender, std::ref(exitFlag));
    std::thread sendVideoThread(SampleSendVideoTask, options, videoFrameSender, std::ref(exitFlag));

    sendAudioThread.join();
    sendVideoThread.join();*/

    return 1;
}

int disconnectAgora()
{
    // Unpublish audio & video track
    connection->getLocalUser()->unpublishAudio(customAudioTrack);
    connection->getLocalUser()->unpublishVideo(customVideoTrack);

    // Disconnect from Agora channel
    if (connection->disconnect()) {
      printf("Failed to disconnect from Agora channel!\n");
      return -1;
    }
    printf("Disconnected from Agora channel successfully\n");

    // Destroy Agora connection and related resources
    connObserver.reset();
    audioPcmDataSender = nullptr;
    videoFrameSender = nullptr;
    customAudioTrack = nullptr;
    customVideoTrack = nullptr;
    factory = nullptr;
    connection = nullptr;

    // Destroy Agora Service
    service->release();
    service = nullptr;
    return 1;
}


int sendOneYuvFrame(void* frameBuf) {
  agora::media::base::ExternalVideoFrame videoFrame;
  videoFrame.type = agora::media::base::ExternalVideoFrame::VIDEO_BUFFER_RAW_DATA;
  videoFrame.format = agora::media::base::VIDEO_PIXEL_I422;
  videoFrame.buffer = frameBuf;
  videoFrame.stride = options.video.width;
  videoFrame.height = options.video.height;
  videoFrame.cropLeft = 0;
  videoFrame.cropTop = 0;
  videoFrame.cropRight = 0;
  videoFrame.cropBottom = 0;
  videoFrame.rotation = 0;
  videoFrame.timestamp = 0;

  if (videoFrameSender->sendVideoFrame(videoFrame) < 0) {
    printf("Failed to send video frame!\n");
    return -1;
  }
  return 1;
}

int sendOnePcmFrame(void* frameBuf) {
  // Calculate byte size for 10ms audio samples
  int sampleSize = sizeof(int16_t) * options.audio.numOfChannels;
  int samplesPer10ms = options.audio.sampleRate / 100;
  int sendBytes = sampleSize * samplesPer10ms;

  // fps = 25   one pcm have 40 ms
  for (int i=0; i<4; i++){
      if (audioPcmDataSender->sendAudioPcmData(frameBuf+i*sendBytes, 0, samplesPer10ms, sampleSize,
                                               options.audio.numOfChannels,
                                               options.audio.sampleRate) < 0) {
        return -1;
      }
  }
  return 1;
}
