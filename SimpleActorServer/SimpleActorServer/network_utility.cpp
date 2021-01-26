#include "network_utility.h"

#include <boost/atomic.hpp>
#include <boost/thread/lock_guard.hpp>
#include <mutex>
#include <unordered_map>

#include "json.h"
#include "logger.h"
#include "session.h"
#include "user_manager.h"

namespace {

boost::detail::spinlock internal_message_lock;
std::unordered_map<int, internal_callback_t> internal_message_handlers;

boost::detail::spinlock message_lock;
std::unordered_map<int, msg_callback_t> message_handlers;

std::atomic<int> session_count = 0;
boost::detail::spinlock session_lock;
std::unordered_map<std::string, std::shared_ptr<Session>> sessions;

}  // namespace

void MessageHandler::RegisterSessionOpened(const internal_callback_t& cb) {
  boost::lock_guard<boost::detail::spinlock> lock{internal_message_lock};
  internal_message_handlers.emplace(static_cast<int>(MessageType::SessionOpen),
                                    cb);
}

void MessageHandler::RegisterSessionClosed(const internal_callback_t& cb) {
  boost::lock_guard<boost::detail::spinlock> lock{internal_message_lock};
  internal_message_handlers.emplace(static_cast<int>(MessageType::SessionClose),
                                    cb);
}

void MessageHandler::RegisterHandler(const int msg_type,
                                     const msg_callback_t& cb) {
  boost::lock_guard<boost::detail::spinlock> lock{message_lock};
  message_handlers.emplace(msg_type, cb);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

int Connection::GetCunnectionCount() { return session_count.load(); }

void Connection::CreateSession(boost::asio::ip::tcp::socket&& socket) {
  auto session = std::make_shared<Session>(std::move(socket));
  session->Start();
  {
    boost::lock_guard<boost::detail::spinlock> lock{session_lock};
    sessions.emplace(session->GetSessionId(), std::move(session));
  }
  session_count.fetch_add(1);
}

void Connection::EraseSession(const std::string& session_id) {
  UserManager::EraseUser(session_id);
  {
    boost::lock_guard<boost::detail::spinlock> lock{session_lock};
    sessions.erase(session_id);
  }
  session_count.fetch_sub(1);
}

bool Connection::IsSessionOpened(const std::string& session_id) {
  boost::lock_guard<boost::detail::spinlock> lock{session_lock};
  return sessions.count(session_id) > 0;
}

std::shared_ptr<Session> Connection::GetSession(const std::string& session_id) {
  boost::lock_guard<boost::detail::spinlock> lock{session_lock};
  const auto& it = sessions.find(session_id);
  if (it == sessions.cend()) {
    return nullptr;
  }
  return it->second;
}

void Connection::DeliverMessage(const std::shared_ptr<Session>& session) {
  Json msg{session->GetMessage()};
  if (not msg.IsObject()) {
    Log::Print(Log::Level::ERROR_,
               "Connection::DeliverMessage(): failed message parse=" +
                   session->GetMessage());
    return;
  }

  int msg_type;
  if (not msg.GetAttribute("msg_type", &msg_type)) {
    Log::Print(Log::Level::ERROR_,
               "Connection::DeliverMessage(): invalid message type=" +
                   session->GetMessage());
    return;
  }

  msg_callback_t cb;
  if (not InternalFunction::GetMessageHandler(msg_type, &cb)) {
    Log::Print(Log::Level::ERROR_,
               "Connection::DeliverMessage(): invalid message handler=" +
                   session->GetMessage());
    return;
  }

  auto user_actor = UserManager::GetUser(session->GetSessionId());
  if (not user_actor) {
    Log::Print(Log::Level::ERROR_,
               "Connection::DeliverMessage(): invalid user actor=" +
                   session->GetSessionId());
    return;
  }

  Json body;
  if (not msg.GetAttribute("msg_body", &body)) {
    Log::Print(Log::Level::ERROR_,
               "Connection::DeliverMessage(): invalid message body=" +
                   session->GetMessage());
    return;
  }

  Log::Print(Log::Level::DEBUG, "Receive Message:" + msg.ToString());

  user_actor->AsyncTask([cb, session, body = std::move(body.ToString())]() {
    Json msg{body};
    cb(session, msg);
  });
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool InternalFunction::GetMessageHandler(const int msg_type,
                                         msg_callback_t* handler) {
  boost::lock_guard<boost::detail::spinlock> lock{message_lock};
  const auto& it = message_handlers.find(msg_type);
  if (it == message_handlers.cend()) {
    *handler = nullptr;
    return false;
  }
  *handler = it->second;
  return true;
}

void InternalFunction::DeliverMessage(const std::shared_ptr<Session>& session,
                                      const Json& msg) {
  int msg_type = msg["msg_type"].GetInt();

  internal_callback_t cb;
  {
    boost::lock_guard<boost::detail::spinlock> lock{internal_message_lock};
    const auto& it = internal_message_handlers.find(msg_type);
    if (it == internal_message_handlers.cend()) {
      return;
    }
    cb = it->second;
  }

  switch (static_cast<MessageType>(msg_type)) {
    case MessageType::SessionOpen: {
      cb(session, nullptr);
    } break;

    case MessageType::SessionClose: {
      auto user_actor = UserManager::GetUser(session->GetSessionId());
      if (not user_actor) {
        return;
      }
      user_actor->AsyncTask(
          [cb, user_actor, session]() { cb(session, user_actor); });
    } break;

    default:
      break;
  }
}