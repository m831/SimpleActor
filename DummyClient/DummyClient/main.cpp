//
// chat_client.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2020 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <cstdlib>
#include <deque>
#include <iostream>

#include "simple_message.h"

using boost::asio::ip::tcp;

typedef std::deque<SimpleMessage> chat_message_queue;

class chat_client {
 public:
  chat_client(boost::asio::io_context& io_context,
              const tcp::resolver::results_type& endpoints)
      : io_context_(io_context), socket_(io_context) {
    do_connect(endpoints);
  }

  void write(const SimpleMessage& msg) {
    boost::asio::post(io_context_, [this, msg]() {
      bool write_in_progress = !write_msgs_.empty();
      write_msgs_.push_back(msg);
      if (!write_in_progress) {
        do_write();
      }
    });
  }

  void close() {
    boost::asio::post(io_context_, [this]() { socket_.close(); });
  }

 private:
  void do_connect(const tcp::resolver::results_type& endpoints) {
    boost::asio::async_connect(
        socket_, endpoints,
        [this](boost::system::error_code ec, tcp::endpoint) {
          if (!ec) {
            do_read_header();
          }
        });
  }

  void do_read_header() {
    boost::asio::async_read(
        socket_,
        boost::asio::buffer(read_msg_.data(), SimpleMessage::header_length),
        [this](boost::system::error_code ec, std::size_t /*length*/) {
          if (!ec && read_msg_.decode_header()) {
            do_read_body();
          } else {
            socket_.close();
          }
        });
  }

  void do_read_body() {
    boost::asio::async_read(
        socket_, boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
        [this](boost::system::error_code ec, std::size_t /*length*/) {
          if (!ec) {
#if !AUTO_BOT
            std::cout.write(read_msg_.body(), read_msg_.body_length());
            std::cout << "\n";
#endif
            do_read_header();
          } else {
            socket_.close();
          }
        });
  }

  void do_write() {
    boost::asio::async_write(
        socket_,
        boost::asio::buffer(write_msgs_.front().data(),
                            write_msgs_.front().length()),
        [this](boost::system::error_code ec, std::size_t /*length*/) {
          if (!ec) {
            write_msgs_.pop_front();
            if (!write_msgs_.empty()) {
              do_write();
            }
          } else {
            socket_.close();
          }
        });
  }

 private:
  boost::asio::io_context& io_context_;
  tcp::socket socket_;
  SimpleMessage read_msg_;
  chat_message_queue write_msgs_;
};

std::atomic<int> counter = 1;

void RunBot(const std::shared_ptr<chat_client>& c) {
  std::string nickname{"bot" +
                       boost::lexical_cast<std::string>(counter.load())};
  Json register_doc;
  register_doc.SetObject();
  register_doc.SetAttribute("nickname", nickname);
  auto register_req = SimpleMessage::MakeMessage(
      static_cast<int>(MessageType::Register), register_doc);
  c->write(register_req);

  Json enter_room_doc;
  enter_room_doc.SetObject();
  auto enter_room_req = SimpleMessage::MakeMessage(
      static_cast<int>(MessageType::EnterRoom), enter_room_doc);
  c->write(enter_room_req);

  std::this_thread::sleep_for(std::chrono::seconds(1));

  try {
    while (true) {
      boost::this_thread::interruption_point();

      std::string blah{"blah... blah... blah..."};
      Json chat_doc;
      chat_doc.SetObject();
      chat_doc.SetAttribute("chat_message",
                            boost::lexical_cast<std::string>(blah));
      auto req = SimpleMessage::MakeMessage(
          static_cast<int>(MessageType::SendChat), chat_doc);
      c->write(req);
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  } catch (...) {
  }
}

#define AUTO_BOT 0
#define AUTO_BOT_SIZE 100

int main(int argc, char* argv[]) {
  try {
    boost::asio::io_context io_context;

#if AUTO_BOT
    tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve("127.0.0.1", "5959");
    std::vector<std::shared_ptr<chat_client>> clients;
    for (int i = 0; i < AUTO_BOT_SIZE; ++i) {
      clients.emplace_back(
          std::make_shared<chat_client>(io_context, endpoints));
    }
    boost::thread t([&io_context]() { io_context.run(); });

    std::vector<std::unique_ptr<boost::thread>> threads;
    for (int i = 0; i < AUTO_BOT_SIZE; ++i) {
      auto client = clients[i];
      threads.emplace_back(std::make_unique<boost::thread>(
          [client]() mutable { RunBot(client); }));
    }
    char line[10];
    while (std::cin.getline(line, 10)) {
      if (line[0] == 'q') {
        break;
      }
    }
    io_context.stop();
    t.join();
    for (auto& thread : threads) {
      if (!!thread && thread->joinable()) {
        thread->interrupt();
        thread->join();
      }
    }
#else
    tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve("127.0.0.1", "5959");
    chat_client c(io_context, endpoints);

    boost::thread t([&io_context]() { io_context.run(); });

    std::cout << "input your nickname:";
    char nickname[50];
    std::cin.getline(nickname, 50);
    Json register_doc;
    register_doc.SetObject();
    register_doc.SetAttribute("nickname", nickname);
    auto register_req = SimpleMessage::MakeMessage(
        static_cast<int>(MessageType::Register), register_doc);
    std::cout << register_req.body() << std::endl;
    c.write(register_req);
    std::cout << "send register" << std::endl;

    Json enter_room_doc;
    enter_room_doc.SetObject();
    auto enter_room_req = SimpleMessage::MakeMessage(
        static_cast<int>(MessageType::EnterRoom), enter_room_doc);
    std::cout << enter_room_req.body() << std::endl;
    c.write(enter_room_req);
    std::cout << "send enter room" << std::endl;

    std::cout << "start chat" << std::endl << std::endl;
    char line[SimpleMessage::max_body_length + 1];
    while (std::cin.getline(line, SimpleMessage::max_body_length + 1)) {
      if (line[0] == 'q') {
        Json exit_doc;
        exit_doc.SetObject();
        auto req = SimpleMessage::MakeMessage(
            static_cast<int>(MessageType::ExitRoom), exit_doc);
        c.write(req);
        Sleep(1000);
        break;
      }

      Json chat_doc;
      chat_doc.SetObject();
      chat_doc.SetAttribute("chat_message",
                            boost::lexical_cast<std::string>(line));
      auto req = SimpleMessage::MakeMessage(
          static_cast<int>(MessageType::SendChat), chat_doc);

      c.write(req);
    }
    c.close();
    io_context.stop();
    t.join();
#endif
  } catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}