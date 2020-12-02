#pragma once
#include "IAgoraService.h"
agora::base::IAgoraService* createAndInitAgoraService(bool enableAudioDevice,
                                                      bool enableAudioProcessor, bool enableVideo);
