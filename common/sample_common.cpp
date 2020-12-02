#include "sample_common.h"

agora::base::IAgoraService* createAndInitAgoraService(bool enableAudioDevice,
                                                      bool enableAudioProcessor, bool enableVideo) {
  auto service = createAgoraService();
  agora::base::AgoraServiceConfiguration scfg;
  scfg.enableAudioProcessor = enableAudioProcessor;
  scfg.enableAudioDevice = enableAudioDevice;
  scfg.enableVideo = enableVideo;

  int ret = service->initialize(scfg);
  return (ret == agora::ERR_OK) ? service : nullptr;
}
