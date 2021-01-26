// Copyright (c) 2003-2020 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include "json.h"

enum class ResultType {
  Error,
  Sucess,
};

enum class MessageType {
  None = 0,
  SessionOpen,
  SessionClose,

  EndOfInternal = 100,

  Register = 101,
  RegisterAck,
  EnterRoom,
  EnterRoomAck,
  ExitRoom,
  ExitRoomAck,
  SendChat,
  SendChatAck,
  BroadcastingChat,
};

class SimpleMessage {
 public:
  enum { header_length = 4 };
  enum { max_body_length = 512 };

  SimpleMessage() : body_length_(0) { memset(data_, 0, sizeof(data_)); }

  const char* data() const { return data_; }

  char* data() { return data_; }

  std::size_t length() const { return header_length + body_length_; }

  const char* body() const { return data_ + header_length; }

  char* body() { return data_ + header_length; }

  std::size_t body_length() const { return body_length_; }

  void body_length(std::size_t new_length) {
    body_length_ = new_length;
    if (body_length_ > max_body_length) body_length_ = max_body_length;
  }

  bool decode_header() {
    char header[header_length + 1] = "";
    std::strncat(header, data_, header_length);
    body_length_ = std::atoi(header);
    if (body_length_ > max_body_length) {
      body_length_ = 0;
      return false;
    }
    return true;
  }

  void encode_header() {
    char header[header_length + 1] = "";
    std::sprintf(header, "%4d", static_cast<int>(body_length_));
    std::memcpy(data_, header, header_length);
  }

  static SimpleMessage MakeMessage(const int msg_type, const Json& doc) {
    Json res_doc;
    res_doc.SetObject();
    res_doc.SetAttribute("msg_type", msg_type);
    res_doc.SetAttribute("msg_body", doc);

    SimpleMessage msg;
    std::string str_res = res_doc.ToString();
    msg.body_length(str_res.size());
    std::memcpy(msg.body(), str_res.c_str(), msg.body_length());
    msg.encode_header();

    return msg;
  }

 private:
  char data_[header_length + max_body_length];
  std::size_t body_length_;
};
