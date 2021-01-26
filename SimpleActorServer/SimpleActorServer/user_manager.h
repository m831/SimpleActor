#pragma once

#include <memory>
#include <string>

#include "user_actor.h"

//:
//: User Actor °ü¸®
//:
namespace UserManager {

void CreateUser(const std::string& session_id);
std::shared_ptr<UserActor> GetUser(const std::string& session_id);
void EraseUser(const std::string& session_id);

}  // namespace UserManager