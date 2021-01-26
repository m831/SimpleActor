#pragma once

#include "actor_base_model.h"

class RoomActor;

class UserActor : public ActorBaseModel {
 public:
  enum class LocalEventType {
    NONE = 0,
    SEND_CHAT_MESSAGE,
  };
  struct LocalEventData : IEventData {
    void InitChatMessage(const std::string& _sender,
                         const std::string& _chat_message) {
      sender = _sender;
      chat_message = _chat_message;
    }
    std::string sender;
    std::string chat_message;
  };

  UserActor(const std::string& session_id);
  virtual ~UserActor();

#pragma region getter setter
  inline std::string GetSessionId() const { return session_id_; }
  inline std::string GetNickname() const { return nickname_; }
  inline void SetNickname(const std::string& nickname) { nickname_ = nickname; }
  inline std::shared_ptr<RoomActor> GetRoomActor() {
    return room_actor_.lock();
  }
  inline void SetRoomActor(std::shared_ptr<RoomActor>& room_actor) {
    room_actor_ = room_actor;
  }
  inline void ClearRoomActor() { room_actor_.reset(); }
#pragma endregion

  virtual void SelfEvent(const int64_t expected_msec);
  virtual void SendAsyncEvent(const int type,
                              const std::shared_ptr<IEventData>&);

  static void InitializeHandler();

 private:
  const std::string session_id_;
  std::string nickname_;
  std::weak_ptr<RoomActor> room_actor_;

  void _SendChatMessage(const std::string& sender,
                        const std::string& chat_message);
};