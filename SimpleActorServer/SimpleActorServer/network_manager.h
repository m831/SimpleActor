#pragma once

#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include "utility.h"

//:
//: boost asio tcp listener
//:
class NetworkManager {
#pragma region singleton pattern
 public:
  static std::shared_ptr<NetworkManager>& GetInstance() {
    std::call_once(NetworkManager::once_flag_,
                   []() { instance_.reset(new NetworkManager(LISTEN_PORT)); });
    return instance_;
  }

  ~NetworkManager();

 private:
  NetworkManager(const short port);

  static std::once_flag once_flag_;
  static std::shared_ptr<NetworkManager> instance_;
#pragma endregion

 public:
  void Start(const size_t thread_count);
  void Stop();

 private:
  bool is_initialize_;
  boost::asio::io_context io_context_;
  boost::asio::ip::tcp::acceptor acceptor_;
  boost::thread_group io_threads_;

  void DoAccept();
};