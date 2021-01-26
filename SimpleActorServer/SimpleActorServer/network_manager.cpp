#include "network_manager.h"

#include <memory>

#include "logger.h"
#include "network_utility.h"

std::once_flag NetworkManager::once_flag_;
std::shared_ptr<NetworkManager> NetworkManager::instance_ = nullptr;

NetworkManager::NetworkManager(const short port)
    : is_initialize_(false),
      acceptor_(boost::asio::ip::tcp::acceptor(
          io_context_,
          boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))) {
  acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
}

NetworkManager::~NetworkManager() {
  Stop();
  Log::Print(Log::Level::INFO, "Destroy NetworkManager...");
}

void NetworkManager::Start(const size_t thread_count) {
  if (is_initialize_) {
    return;
  }

  for (size_t i = 0; i < thread_count; ++i) {
    io_threads_.create_thread(
        boost::bind(&boost::asio::io_service::run, &io_context_));
  }

  DoAccept();
  is_initialize_ = true;
}

void NetworkManager::Stop() {
  io_context_.stop();
  io_threads_.join_all();
  io_context_.reset();
  is_initialize_ = false;
}

void NetworkManager::DoAccept() {
  acceptor_.async_accept([this](boost::system::error_code ec,
                                boost::asio::ip::tcp::socket socket) {
    if (not ec) {
      Connection::CreateSession(std::move(socket));
    }

    DoAccept();
  });
}