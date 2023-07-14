#include "single_instance.h"

#if defined(_WIN32)
#include <Windows.h>
#elif defined(__APPLE__) || defined(__linux__)
#include <fcntl.h>
#include <unistd.h>
#endif

namespace vector_audio {

  const std::string INSTANCE_KEY = "org.vatsim.vectoraudio";

  struct SingleInstance::instance {
    // Indicates whether another instance is running.
    bool exists = false;
#if defined(_WIN32)
    // Blocking mutex on Windows platforms.
    HANDLE mutex = nullptr;
#elif defined(__APPLE__) || defined(__linux__)
    // Blocking file on Unix platforms.
    int blockingFile = -1;
#endif
  };

  SingleInstance::SingleInstance() : instance_(new instance) {
#if defined(_WIN32)
    instance_->mutex = CreateMutexA(NULL, FALSE, INSTANCE_KEY.c_str());
    instance_->exists = GetLastError() == ERROR_ALREADY_EXISTS;
#elif defined(__APPLE__) || defined(__linux__)
    instance_->blockingFile = open(("/tmp/" + INSTANCE_KEY).c_str(), O_CREAT | O_RDWR, 0666);
    struct flock lock {};
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_CUR;
    lock.l_start = 0;
    lock.l_len = 0;
    instance_->exists = fcntl(instance_->blockingFile, F_SETLK, &lock) < 0;
#endif
  }

  SingleInstance::~SingleInstance() {
#if defined(_WIN32)
    if(instance_->mutex) {
      CloseHandle(instance_->mutex);
    }
    instance_->mutex = nullptr;
    instance_->exists = false;
#elif defined(__linux__) || defined(__APPLE__)
    close(instance_->blockingFile);
    instance_->blockingFile = -1;
    instance_->exists = false;
#endif
  }

  bool SingleInstance::HasRunningInstance() const {
    return instance_->exists;
  }
}