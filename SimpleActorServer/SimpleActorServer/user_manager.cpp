#include "user_manager.h"

#include <boost/atomic.hpp>
#include <boost/date_time.hpp>
#include <boost/thread/lock_guard.hpp>
#include <unordered_map>

namespace {

boost::detail::spinlock user_lock;
std::unordered_map<std::string, std::shared_ptr<UserActor>> users;

}  // namespace

void UserManager::CreateUser(const std::string& session_id) {
  auto user_actor = std::make_shared<UserActor>(session_id);
  user_actor->StartRecursiveEvent(10000);
  {
    boost::lock_guard<boost::detail::spinlock> lock{user_lock};
    users.emplace(session_id, std::move(user_actor));
  }
}

std::shared_ptr<UserActor> UserManager::GetUser(const std::string& session_id) {
  std::shared_ptr<UserActor> user_acotr = nullptr;
  {
    boost::lock_guard<boost::detail::spinlock> lock{user_lock};
    const auto& it = users.find(session_id);
    if (it != users.cend()) user_acotr = it->second;
  }
  return user_acotr;
}

void UserManager::EraseUser(const std::string& session_id) {
  boost::lock_guard<boost::detail::spinlock> lock{user_lock};
  users.erase(session_id);
}
