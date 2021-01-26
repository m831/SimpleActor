#include "user_actor.h"

#include <memory>

#include "json.h"
#include "logger.h"
#include "network_utility.h"
#include "room_manager.h"
#include "session.h"
#include "simple_message.h"
#include "user_manager.h"

namespace {

void OnOpenedSession(const std::shared_ptr<Session>& session,
                     const std::shared_ptr<ActorBaseModel>& /*nullptr*/) {
  UserManager::CreateUser(session->GetSessionId());
}

void OnClosedSession(const std::shared_ptr<Session>& session,
                     const std::shared_ptr<ActorBaseModel>& actor) {
  auto user_actor = std::dynamic_pointer_cast<UserActor>(actor);
  if (not user_actor) {
    return;
  }
  auto room_actor = user_actor->GetRoomActor();
  if (not room_actor) {
    return;
  }

  auto data = std::make_shared<RoomActor::LocalEventData>();
  data->InitExitRoom(user_actor->GetSessionId());
  room_actor->SendAsyncEvent(
      static_cast<int>(RoomActor::LocalEventType::EXIT_ROOM), data);
}

void OnRegisterUser(const std::shared_ptr<Session>& session,
                    const Json& request) {
  auto user_actor = UserManager::GetUser(session->GetSessionId());
  if (not user_actor) {
    Session::SendErrorMessage(session, MessageType::RegisterAck,
                              ResultType::Error);
    return;
  }

  std::string nickname;
  if (not request.GetAttribute("nickname", &nickname)) {
    Session::SendErrorMessage(session, MessageType::RegisterAck,
                              ResultType::Error);
    return;
  }

  user_actor->SetNickname(nickname);

  Json res_doc;
  res_doc.SetObject();
  res_doc.SetAttribute("result", static_cast<int>(ResultType::Sucess));
  auto response = SimpleMessage::MakeMessage(
      static_cast<int>(MessageType::RegisterAck), res_doc);
  session->SendMessage(response);
}

void OnEnterRoom(const std::shared_ptr<Session>& session, const Json& request) {
  auto user_actor = UserManager::GetUser(session->GetSessionId());
  if (not user_actor) {
    Session::SendErrorMessage(session, MessageType::EnterRoomAck,
                              ResultType::Error);
    return;
  }

  auto room_actor = RoomManager::GetRandomRoom();
  if (not room_actor) {
    Session::SendErrorMessage(session, MessageType::EnterRoomAck,
                              ResultType::Error);
    return;
  }

  auto data = std::make_shared<RoomActor::LocalEventData>();
  data->InitEnterRoom(user_actor);
  room_actor->SendAsyncEvent(
      static_cast<int>(RoomActor::LocalEventType::ENTER_ROOM), data);

  user_actor->SetRoomActor(room_actor);

  Json res_doc;
  res_doc.SetObject();
  res_doc.SetAttribute("result", static_cast<int>(ResultType::Sucess));
  auto response = SimpleMessage::MakeMessage(
      static_cast<int>(MessageType::EnterRoomAck), res_doc);
  session->SendMessage(response);
}

void OnExitRoom(const std::shared_ptr<Session>& session, const Json& request) {
  auto user_actor = UserManager::GetUser(session->GetSessionId());
  if (not user_actor) {
    Session::SendErrorMessage(session, MessageType::ExitRoomAck,
                              ResultType::Error);
    return;
  }

  auto room_actor = user_actor->GetRoomActor();
  if (not room_actor) {
    Session::SendErrorMessage(session, MessageType::ExitRoomAck,
                              ResultType::Error);
    return;
  }

  auto data = std::make_shared<RoomActor::LocalEventData>();
  data->InitExitRoom(user_actor->GetSessionId());
  room_actor->SendAsyncEvent(
      static_cast<int>(RoomActor::LocalEventType::EXIT_ROOM), data);

  user_actor->ClearRoomActor();

  Json res_doc;
  res_doc.SetObject();
  res_doc.SetAttribute("result", static_cast<int>(ResultType::Sucess));
  auto response = SimpleMessage::MakeMessage(
      static_cast<int>(MessageType::ExitRoomAck), res_doc);
  session->SendMessage(response);
}

void OnSendChatMessage(const std::shared_ptr<Session>& session,
                       const Json& request) {
  auto user_actor = UserManager::GetUser(session->GetSessionId());
  if (not user_actor) {
    Session::SendErrorMessage(session, MessageType::SendChatAck,
                              ResultType::Error);
    return;
  }

  std::string chat_message;
  if (not request.GetAttribute("chat_message", &chat_message)) {
    Session::SendErrorMessage(session, MessageType::SendChatAck,
                              ResultType::Error);
    return;
  }

  auto room_actor = user_actor->GetRoomActor();
  if (not room_actor) {
    Session::SendErrorMessage(session, MessageType::SendChatAck,
                              ResultType::Error);
    return;
  }

  auto data = std::make_shared<RoomActor::LocalEventData>();
  data->InitBroadcating(user_actor->GetNickname(), chat_message);
  room_actor->SendAsyncEvent(
      static_cast<int>(RoomActor::LocalEventType::BROADCASTING), data);

  Json res_doc;
  res_doc.SetObject();
  res_doc.SetAttribute("result", static_cast<int>(ResultType::Sucess));
  auto response = SimpleMessage::MakeMessage(
      static_cast<int>(MessageType::SendChatAck), res_doc);
  session->SendMessage(response);
}

}  // namespace

void UserActor::InitializeHandler() {
  MessageHandler::RegisterSessionOpened(OnOpenedSession);
  MessageHandler::RegisterSessionClosed(OnClosedSession);

  MessageHandler::RegisterHandler(static_cast<int>(MessageType::Register),
                                  OnRegisterUser);
  MessageHandler::RegisterHandler(static_cast<int>(MessageType::EnterRoom),
                                  OnEnterRoom);
  MessageHandler::RegisterHandler(static_cast<int>(MessageType::ExitRoom),
                                  OnExitRoom);
  MessageHandler::RegisterHandler(static_cast<int>(MessageType::SendChat),
                                  OnSendChatMessage);
}

UserActor::UserActor(const std::string& session_id)
    : session_id_(session_id), nickname_("") {}

UserActor::~UserActor() {}

void UserActor::SelfEvent(const int64_t expected_msec) {
  ActorBaseModel::SelfEvent(expected_msec);
  // TODO
}

void UserActor::SendAsyncEvent(const int type,
                               const std::shared_ptr<IEventData>& data) {
  auto local_data = std::dynamic_pointer_cast<LocalEventData>(data);
  if (not local_data) {
    Log::Print(Log::Level::ERROR_, "UserActor::SendAsyncEvent(): invalid data");
    return;
  }

  auto self(shared_from_this());
  switch (static_cast<LocalEventType>(type)) {
    case LocalEventType::SEND_CHAT_MESSAGE:
      this->AsyncTask([this, self, data = std::move(local_data)] {
        _SendChatMessage(data->sender, data->chat_message);
      });
      break;

    default:
      Log::Print(Log::Level::ERROR_, "UserActor::SendAsyncEvent(" +
                                         std::to_string(type) +
                                         "): invalid type");
      break;
  }
}

void UserActor::_SendChatMessage(const std::string& sender,
                                 const std::string& chat_message) {
  if (sender == nickname_) {
    return;
  }

  auto session = Connection::GetSession(session_id_);
  if (not session) {
    return;
  }

  Json res_doc;
  res_doc.SetObject();
  res_doc.SetAttribute("result", static_cast<int>(ResultType::Sucess));
  res_doc.SetAttribute("sender", sender);
  res_doc.SetAttribute("chat_message", chat_message);
  auto response = SimpleMessage::MakeMessage(
      static_cast<int>(MessageType::BroadcastingChat), res_doc);
  session->SendMessage(response);
}