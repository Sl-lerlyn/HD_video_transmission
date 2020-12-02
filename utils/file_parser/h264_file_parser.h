#pragma once

#include <stdio.h>
#include "media_file_parser.h"

class H264FileParser : public VideoFileParser {
 public:
  explicit H264FileParser(const char* filepath);
  virtual ~H264FileParser();

  bool open() override;
  bool hasNext() override;
  void getNext(char* buffer, int* length) override;
  void close();

 private:
  void readData();

 private:
  static constexpr int BufferSize = 409600;

  char* filePath_;
  FILE* fileHandle_;
  unsigned char dataBuffer_[BufferSize] = {0};
  bool isEof_;
  int currentBytePos_;
  int dataEndPos_;
  int currentFrameStart_;

  int readsize_;
};
