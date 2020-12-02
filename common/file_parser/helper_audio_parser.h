#include <string>
#include "NGIAgoraMediaNodeFactory.h"
#include "utils/file_parser/audio_file_parser_factory.h"

struct HelperAudioFrame {
  agora::rtc::EncodedAudioFrameInfo audioFrameInfo;
  std::unique_ptr<char[]> buffer;
  int bufferLen;
};

class HelperAudioFileParser {
 public:
  HelperAudioFileParser(const char* filepath, AUDIO_FILE_TYPE filetype);

  ~HelperAudioFileParser();

  bool initialize();

  std::unique_ptr<HelperAudioFrame> getAudioFrame(int frameSizeDuration);

 private:
  std::string file_path;
  AUDIO_FILE_TYPE file_type;
  agora::agora_refptr<agora::rtc::IAudioEncodedFrameSender> audioOpusFrameSender;
  std::unique_ptr<AudioFileParser> file_parser_;
  int64_t sent_audio_frames_{0};
};
