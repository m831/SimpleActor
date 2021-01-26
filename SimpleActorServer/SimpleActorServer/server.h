#pragma once

#include <boost/thread.hpp>
#include <memory>

#include "logger.h"

class Server {
 public:
  Server();
  ~Server();

  void Start();
  void Stop();

 private:
  std::unique_ptr<boost::thread> thread_;

  void Initialize();
};