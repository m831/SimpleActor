#pragma once

#include <string>

#include "../rapidjson/document.h"
#include "../rapidjson/stringbuffer.h"
#include "../rapidjson/writer.h"

//:
//: rapidjson::Document 래핑 클래스
//:
class Json : public rapidjson::Document {
 public:
  Json() {}
  Json(const std::string& str) { Parse(str.c_str()); }
  Json(const Json& json) { Parse(json.ToString().c_str()); }
  Json(const rapidjson::Document& doc) { Parse(Json::ToString(doc).c_str()); }
  Json(const rapidjson::Value& doc) { Parse(Json::ToString(doc).c_str()); }
  ~Json() = default;

#pragma region getter setter
  bool GetAttribute(const std::string& key, int* return_value) const {
    auto k = rapidjson::StringRef(key.c_str());
    auto itr = this->FindMember(k);
    if (itr == this->MemberEnd()) return false;

    *return_value = itr->value.GetInt();
    return true;
  }
  bool GetAttribute(const std::string& key, std::string* return_value) const {
    auto k = rapidjson::StringRef(key.c_str());
    auto itr = this->FindMember(k);
    if (itr == this->MemberEnd()) return false;

    *return_value = itr->value.GetString();
    return true;
  }
  bool GetAttribute(const std::string& key, Json* return_value) const {
    auto k = rapidjson::StringRef(key.c_str());
    auto itr = this->FindMember(k);
    if (itr == this->MemberEnd()) return false;

    return_value->Parse(Json::ToString(itr->value).c_str());
    return true;
  }

  void SetAttribute(const std::string& key, const int value) {
    auto& allocator = this->GetAllocator();
    rapidjson::Value k(key.c_str(), allocator);
    this->AddMember(k, value, allocator);
  }
  void SetAttribute(const std::string& key, const std::string& value) {
    auto& allocator = this->GetAllocator();
    rapidjson::Value k(key.c_str(), allocator);
    rapidjson::Value v(value.c_str(), allocator);
    this->AddMember(k, v, allocator);
  }
  void SetAttribute(const std::string& key, const Json& value) {
    auto& allocator = this->GetAllocator();
    rapidjson::Value k(key.c_str(), allocator);
    rapidjson::Document v(&allocator);
    v.Parse(value.ToString().c_str());
    this->AddMember(k, v, allocator);
  }
#pragma endregion

  std::string ToString() const {
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    this->Accept(writer);
    return sb.GetString();
  }

  static std::string ToString(const rapidjson::Value& value) {
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    value.Accept(writer);
    return sb.GetString();
  }

  static std::string ToString(const rapidjson::Document& value) {
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    value.Accept(writer);
    return sb.GetString();
  }
};