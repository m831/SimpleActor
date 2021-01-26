#pragma once

#include <memory>
#include <unordered_map>

#include "actor_base_model.h"

class UserActor;

class RoomActor : public ActorBaseModel {
 public:
  enum class LocalEventType {
    NONE = 0,
    ENTER_ROOM,
    EXIT_ROOM,
    BROADCASTING,
  };
  struct LocalEventData : IEventData {
    void InitEnterRoom(const std::shared_ptr<UserActor> &_user) {
      user = _user;
    }
    void InitExitRoom(const std::string &session_id_) {
      session_id = session_id_;
    }
    void InitBroadcating(const std::string &_sender,
                         const std::string &_chat_message) {
      sender = _sender;
      chat_message = _chat_message;
    }
    std::shared_ptr<UserActor> user;
    std::string session_id;
    std::string sender;
    std::string chat_message;
  };

  RoomActor();
  virtual ~RoomActor();

  inline std::string GetRoomId() const { return room_id_; }

  virtual void SelfEvent(const int64_t expected_msec);
  virtual void SendAsyncEvent(const int type,
                              const std::shared_ptr<IEventData> &);

 private:
  const std::string room_id_;
  std::unordered_map<std::string, std::weak_ptr<UserActor>> members_;

  void _EnterRoom(const std::shared_ptr<UserActor> &user);
  void _ExitRoom(const std::string &session_id);
  void _Broadcasting(const std::string &sender,
                     const std::string &chat_message);
};