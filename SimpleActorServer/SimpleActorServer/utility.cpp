#include "utility.h"

#include <boost/date_time.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

std::string Utility::GenerateStringUuid() {
  boost::uuids::uuid uuid = boost::uuids::random_generator()();
  return boost::lexical_cast<std::string>(uuid);
}

int Utility::RandomGenerateNumber(const int start, const int end) {
  srand((unsigned)time(NULL));
  return rand() % end + start;
}

int64_t Utility::GetMillisTimestampFromNow() {
  boost::posix_time::ptime time_t_epoch(boost::gregorian::date(1970, 1, 1));
  boost::posix_time::ptime now =
      boost::posix_time::microsec_clock::local_time();
  boost::posix_time::time_duration diff = now - time_t_epoch;
  return diff.total_milliseconds();
}

int64_t Utility::GetTimestampFromNow() {
  boost::posix_time::ptime time_t_epoch(boost::gregorian::date(1970, 1, 1));
  boost::posix_time::ptime now =
      boost::posix_time::microsec_clock::local_time();
  boost::posix_time::time_duration diff = now - time_t_epoch;
  return diff.total_seconds();
}