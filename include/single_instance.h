#pragma once

#include <memory>
#include <string>

namespace vector_audio {
  class SingleInstance {
    public:
      explicit SingleInstance();
      ~SingleInstance();
      bool HasRunningInstance() const;
    private:
      struct instance;
      std::unique_ptr<instance> instance_;
  };
}