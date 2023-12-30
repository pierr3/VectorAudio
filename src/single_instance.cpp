#include "single_instance.h"
#include <cstdio>
#include <string>

#if defined(_WIN32)
#include <Windows.h>
#elif defined(__APPLE__) || defined(__linux__)
#include <fcntl.h>
#include <unistd.h>
#endif

namespace vector_audio {

  const std::string kInstanceKey = "org.vatsim.vectoraudio";

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

  SingleInstance::SingleInstance() : pInstance(new instance) {
#if defined(_WIN32)
    pInstance->mutex = CreateMutexA(NULL, FALSE, INSTANCE_KEY.c_str());
    pInstance->exists = GetLastError() == ERROR_ALREADY_EXISTS;
#elif defined(__APPLE__) || defined(__linux__)
    pInstance->blockingFile = open(("/tmp/" + kInstanceKey).c_str(), O_CREAT | O_RDWR, 0666);
    struct flock lock {};
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_CUR;
    lock.l_start = 0;
    lock.l_len = 0;
    pInstance->exists = fcntl(pInstance->blockingFile, F_SETLK, &lock) < 0;
#endif
  }

  SingleInstance::~SingleInstance() {
#if defined(_WIN32)
    if(pInstance->mutex) {
      CloseHandle(pInstance->mutex);
    }
    pInstance->mutex = nullptr;
    pInstance->exists = false;
#elif defined(__linux__) || defined(__APPLE__)
    close(pInstance->blockingFile);
    pInstance->blockingFile = -1;
    pInstance->exists = false;
#endif
  }

  bool SingleInstance::HasRunningInstance() const {
    return pInstance->exists;
  }
}