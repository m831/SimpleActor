#pragma once

#include <boost/asio.hpp>
#include <deque>

#include "simple_message.h"

//:
//: tcp socket session
//:
class Session : public std::enable_shared_from_this<Session> {
 public:
  Session(boost::asio::ip::tcp::socket&& socket);
  ~Session();

  inline std::string GetSessionId() const { return session_id_; }
  inline const std::string GetMessage() const {
    return std::string(read_msg_.body()).substr(0, read_msg_.body_length());
  }

  void Start();
  void SendMessage(const SimpleMessage& msg);

  static void SendErrorMessage(const std::shared_ptr<Session>&,
                               const MessageType msg_type,
                               const ResultType result_type);

 private:
  const std::string session_id_;
  boost::asio::ip::tcp::socket socket_;
  SimpleMessage read_msg_;

  std::deque<SimpleMessage> write_msgs_;

  void OnClosed();
  void DoReadHeader();
  void DoReadBody();
  void DoWrite();
};