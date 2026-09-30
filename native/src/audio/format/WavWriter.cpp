#include "audio/format/WavWriter.hpp"

namespace shareguard {
namespace {

bool writeExact(HANDLE file, const void* data, DWORD size, std::string& error) {
  DWORD written = 0;
  if (!WriteFile(file, data, size, &written, nullptr) || written != size) {
    error = "Failed to write WAV file.";
    return false;
  }
  return true;
}

}

WavWriter::~WavWriter() {
  std::string ignored;
  if (open_) {
    close(ignored);
  }
}

bool WavWriter::open(const std::wstring& path, std::string& error) {
  file_.reset(CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));
  if (!file_) {
    error = "Failed to create WAV file.";
    return false;
  }

  const uint32_t sampleRate = 48000;
  const uint16_t channels = 2;
  const uint16_t bits = 16;
  const uint16_t blockAlign = channels * bits / 8;
  const uint32_t byteRate = sampleRate * blockAlign;
  uint8_t header[44] = {};
  header[0] = 'R';
  header[1] = 'I';
  header[2] = 'F';
  header[3] = 'F';
  header[8] = 'W';
  header[9] = 'A';
  header[10] = 'V';
  header[11] = 'E';
  header[12] = 'f';
  header[13] = 'm';
  header[14] = 't';
  header[15] = ' ';
  header[16] = 16;
  header[20] = 1;
  header[22] = static_cast<uint8_t>(channels);
  header[24] = static_cast<uint8_t>(sampleRate);
  header[25] = static_cast<uint8_t>(sampleRate >> 8);
  header[26] = static_cast<uint8_t>(sampleRate >> 16);
  header[27] = static_cast<uint8_t>(sampleRate >> 24);
  header[28] = static_cast<uint8_t>(byteRate);
  header[29] = static_cast<uint8_t>(byteRate >> 8);
  header[30] = static_cast<uint8_t>(byteRate >> 16);
  header[31] = static_cast<uint8_t>(byteRate >> 24);
  header[32] = static_cast<uint8_t>(blockAlign);
  header[34] = static_cast<uint8_t>(bits);
  header[36] = 'd';
  header[37] = 'a';
  header[38] = 't';
  header[39] = 'a';
  if (!writeExact(file_.get(), header, sizeof(header), error)) {
    return false;
  }
  open_ = true;
  dataBytes_ = 0;
  return true;
}

bool WavWriter::write(const uint8_t* pcm, size_t size, std::string& error) {
  if (!open_ || pcm == nullptr || size == 0) {
    return true;
  }
  if (size > 0xFFFFFFFF) {
    error = "WAV data is too large.";
    return false;
  }
  if (!writeExact(file_.get(), pcm, static_cast<DWORD>(size), error)) {
    return false;
  }
  dataBytes_ += static_cast<uint32_t>(size);
  return true;
}

bool WavWriter::close(std::string& error) {
  if (!open_) {
    return true;
  }
  const uint32_t riffSize = dataBytes_ + 36;
  auto patch = [&](DWORD offset, uint32_t value) {
    if (SetFilePointer(file_.get(), offset, nullptr, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
      error = "Failed to finalize WAV header.";
      return false;
    }
    return writeExact(file_.get(), &value, sizeof(value), error);
  };
  const bool patched = patch(4, riffSize) && patch(40, dataBytes_);
  FlushFileBuffers(file_.get());
  file_.reset();
  open_ = false;
  return patched;
}

}
