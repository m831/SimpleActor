#pragma once

#include <boost/asio.hpp>
#include <memory>

#include "json.h"

class ActorBaseModel;
class Session;

using internal_callback_t = std::function<void(
    const std::shared_ptr<Session>&, const std::shared_ptr<ActorBaseModel>)>;

using msg_callback_t =
    std::function<void(const std::shared_ptr<Session>&, const Json&)>;

namespace MessageHandler {

void RegisterSessionOpened(const internal_callback_t& cb);
void RegisterSessionClosed(const internal_callback_t& cb);

void RegisterHandler(const int msg_type, const msg_callback_t& cb);

}  // namespace MessageHandler

namespace Connection {

int GetCunnectionCount();

void CreateSession(boost::asio::ip::tcp::socket&& socket);
void EraseSession(const std::string& session_id);
bool IsSessionOpened(const std::string& session_id);
std::shared_ptr<Session> GetSession(const std::string& session_id);

void DeliverMessage(const std::shared_ptr<Session>& session);

}  // namespace Connection

namespace InternalFunction {

bool GetMessageHandler(const int msg_type, msg_callback_t* handler);

void DeliverMessage(const std::shared_ptr<Session>& session, const Json& msg);

}  // namespace InternalFunction