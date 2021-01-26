#include "room_manager.h"

#include <boost/atomic.hpp>
#include <boost/date_time.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread/lock_guard.hpp>
#include <unordered_map>

namespace {

boost::detail::spinlock room_lock;
std::unordered_map<std::string, std::shared_ptr<RoomActor>> rooms;

}  // namespace

std::shared_ptr<RoomActor> RoomManager::CreateRoom() {
  auto room_actor = std::make_shared<RoomActor>();
  room_actor->StartRecursiveEvent(10000);

  {
    boost::lock_guard<boost::detail::spinlock> lock{room_lock};
    rooms.emplace(room_actor->GetRoomId(), room_actor);
  }

  return room_actor;
}

std::shared_ptr<RoomActor> RoomManager::GetRandomRoom() {
  boost::lock_guard<boost::detail::spinlock> lock{room_lock};
  if (rooms.empty()) {
    return nullptr;
  }
  auto random_it = std::next(
      std::begin(rooms),
      Utility::RandomGenerateNumber(0, static_cast<int>(rooms.size())));
  return random_it->second;
}

std::shared_ptr<RoomActor> RoomManager::GetRoom(const std::string& room_id) {
  std::shared_ptr<RoomActor> room_acotr = nullptr;
  {
    boost::lock_guard<boost::detail::spinlock> lock{room_lock};
    const auto& it = rooms.find(room_id);
    if (it != rooms.cend()) room_acotr = it->second;
  }
  return room_acotr;
}

void RoomManager::EraseRoom(const std::string& room_id) {
  boost::lock_guard<boost::detail::spinlock> lock{room_lock};
  rooms.erase(room_id);
}
