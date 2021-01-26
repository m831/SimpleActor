#include "room_actor.h"

#include "logger.h"
#include "user_manager.h"

RoomActor::RoomActor() : room_id_(Utility::GenerateStringUuid()) {
  Log::Print(Log::Level::DEBUG, "CreateRoom...");
}

RoomActor::~RoomActor() { Log::Print(Log::Level::DEBUG, "DestoryRoom..."); }

void RoomActor::SelfEvent(const int64_t expected_msec) {
  ActorBaseModel::SelfEvent(expected_msec);
  // TODO
}

void RoomActor::SendAsyncEvent(const int type,
                               const std::shared_ptr<IEventData>& data) {
  auto local_data = std::dynamic_pointer_cast<LocalEventData>(data);
  if (not local_data) {
    Log::Print(Log::Level::ERROR_, "RoomActor::SendAsyncEvent(): invalid data");
    return;
  }

  auto self(shared_from_this());
  switch (static_cast<LocalEventType>(type)) {
    case LocalEventType::ENTER_ROOM:
      this->AsyncTask([this, self, data = std::move(local_data)]() {
        _EnterRoom(data->user);
      });
      break;

    case LocalEventType::EXIT_ROOM:
      this->AsyncTask([this, self, data = std::move(local_data)]() {
        _ExitRoom(data->session_id);
      });
      break;

    case LocalEventType::BROADCASTING:
      this->AsyncTask([this, self, data = std::move(local_data)]() {
        _Broadcasting(data->sender, data->chat_message);
      });
      break;

    default:
      Log::Print(Log::Level::ERROR_, "RoomActor::SendAsyncEvent(" +
                                         std::to_string(type) +
                                         "): invalid type");
      break;
  }
}

void RoomActor::_EnterRoom(const std::shared_ptr<UserActor>& user) {
  Log::Print(Log::Level::DEBUG, "User EnterRoom");
  members_.emplace(user->GetSessionId(), user);
}

void RoomActor::_ExitRoom(const std::string& session_id) {
  Log::Print(Log::Level::DEBUG, "User ExitRoom");
  members_.erase(session_id);
}

void RoomActor::_Broadcasting(const std::string& sender,
                              const std::string& chat_message) {
  for (const auto& entry : members_) {
    auto user_actor = entry.second.lock();
    if (not user_actor) {
      continue;
    }
    auto data = std::make_shared<UserActor::LocalEventData>();
    data->InitChatMessage(sender, chat_message);
    user_actor->SendAsyncEvent(
        static_cast<int>(UserActor::LocalEventType::SEND_CHAT_MESSAGE), data);
  }
}