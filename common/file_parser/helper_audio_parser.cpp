#include "helper_audio_parser.h"

#include <cstring>
#include <vector>

HelperAudioFileParser::HelperAudioFileParser(const char* filepath, AUDIO_FILE_TYPE filetype)
    : file_path(filepath), file_type(filetype) {}

HelperAudioFileParser::~HelperAudioFileParser() = default;

bool HelperAudioFileParser::initialize() {
  AudioFileParserFactory::ParserConfig config;
  config.filePath = file_path.c_str();
  config.fileType = file_type;
  file_parser_ = std::move(AudioFileParserFactory::Instance().createAudioFileParser(config));
  if (!file_parser_ || !file_parser_->open()) {
    printf("Open opus file %s failed\n", file_path.c_str());
    return false;
  }
  printf("Open opus file %s successfully\n", file_path.c_str());
  return true;
}

std::unique_ptr<HelperAudioFrame> HelperAudioFileParser::getAudioFrame(int frameSizeDuration) {
  std::unique_ptr<HelperAudioFrame> audioFrame = nullptr;
  static uint8_t databuf[8192] = {0};
  static int length = 8192;
  static int bytesnum = 0;
  agora::rtc::EncodedAudioFrameInfo audioFrameInfo;
  audioFrameInfo.numberOfChannels = file_parser_->getNumberOfChannels();
  audioFrameInfo.sampleRateHz = file_parser_->getSampleRateHz();
  audioFrameInfo.codec = file_parser_->getCodecType();
  // calculate Opus frame size
  audioFrameInfo.samplesPerChannel = file_parser_->getSampleRateHz() * frameSizeDuration /1000 ;

  if (!file_parser_->hasNext()) {
    file_parser_->reset();
    return nullptr;
  }
  length = 8192;
  file_parser_->getNext(reinterpret_cast<char*>(databuf), &length);
  if (length > 0) {
    std::unique_ptr<char[]> buffer2(new char[length]);
    memcpy(buffer2.get(), databuf, length);
    audioFrame.reset(new HelperAudioFrame{audioFrameInfo, std::move(buffer2), length});

    bytesnum += length;
  }
  return audioFrame;
}
