#include "session.h"

#include <memory>

#include "json.h"
#include "logger.h"
#include "network_utility.h"
#include "user_manager.h"
#include "utility.h"

Session::Session(boost::asio::ip::tcp::socket&& socket)
    : session_id_(Utility::GenerateStringUuid()), socket_(std::move(socket)) {
  Log::Print(Log::Level::DEBUG, "Create Session");
}

Session::~Session() { Log::Print(Log::Level::DEBUG, "Close Session"); }

void Session::SendMessage(const SimpleMessage& msg) {
  bool write_in_progress = not write_msgs_.empty();
  write_msgs_.push_back(msg);
  if (not write_in_progress) {
    DoWrite();
  }
}

void Session::SendErrorMessage(const std::shared_ptr<Session>& session,
                               const MessageType msg_type,
                               const ResultType result_type) {
  Json res_doc;
  res_doc.SetObject();
  res_doc.SetAttribute("result", static_cast<int>(result_type));
  auto response =
      SimpleMessage::MakeMessage(static_cast<int>(msg_type), res_doc);
  session->SendMessage(response);
}

void Session::Start() {
  Json msg;
  msg.SetObject();
  msg.SetAttribute("msg_type", static_cast<int>(MessageType::SessionOpen));
  InternalFunction::DeliverMessage(shared_from_this(), msg);

  DoReadHeader();
}

void Session::OnClosed() {
  if (Connection::IsSessionOpened(session_id_)) {
    Json msg;
    msg.SetObject();
    msg.SetAttribute("msg_type", static_cast<int>(MessageType::SessionClose));
    InternalFunction::DeliverMessage(shared_from_this(), msg);

    Connection::EraseSession(session_id_);
  }
}

void Session::DoReadHeader() {
  auto self(shared_from_this());
  boost::asio::async_read(
      socket_,
      boost::asio::buffer(read_msg_.data(), SimpleMessage::header_length),
      [this, self](boost::system::error_code ec, std::size_t /*length*/) {
        if (!ec && read_msg_.decode_header()) {
          DoReadBody();
        } else {
          OnClosed();
        }
      });
}

void Session::DoReadBody() {
  auto self(shared_from_this());
  boost::asio::async_read(
      socket_, boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
      [this, self](boost::system::error_code ec, std::size_t /*length*/) {
        if (!ec) {
          Connection::DeliverMessage(self);
          DoReadHeader();
        } else {
          OnClosed();
        }
      });
}

void Session::DoWrite() {
  auto self(shared_from_this());
  boost::asio::async_write(
      socket_,
      boost::asio::buffer(write_msgs_.front().data(),
                          write_msgs_.front().length()),
      [this, self](boost::system::error_code ec, std::size_t /*length*/) {
        if (!ec) {
          write_msgs_.pop_front();
          if (!write_msgs_.empty()) {
            DoWrite();
          }
        } else {
          OnClosed();
        }
      });
}