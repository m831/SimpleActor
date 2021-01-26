#pragma once

#include <memory>
#include <string>

#include "room_actor.h"

//:
//: Room Actor °ü¸®
//:
namespace RoomManager {

std::shared_ptr<RoomActor> CreateRoom();
std::shared_ptr<RoomActor> GetRandomRoom();
std::shared_ptr<RoomActor> GetRoom(const std::string& room_id);
void EraseRoom(const std::string& room_id);

}  // namespace RoomManager