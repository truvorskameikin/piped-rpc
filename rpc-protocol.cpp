#include <iostream>
#include "rpc-protocol.h"

namespace rpc_protocol {
  namespace detail {
    const char* ReadString(
      const char* bytes, const char* end,
      std::string& string_out) {

      if (bytes + sizeof(int32_t) > end)
        return nullptr;

      int32_t string_length = *(int32_t*) bytes;
      bytes += sizeof(int32_t);

      if (bytes + string_length > end)
        return nullptr;

      string_out.clear();
      string_out.insert(string_out.end(), bytes, bytes + string_length);
      bytes += string_length;

      return bytes;
    }

    const char* ReadValue(
      const char* bytes, const char* end,
      Value& value_out) {

      if (bytes + sizeof(int8_t) > end)
        return nullptr;

      int8_t type = *(int8_t*) bytes;
      bytes += sizeof(int8_t);

      if (type != Value::kInt32 && type != Value::kString)
        return nullptr;

      if (type == Value::kInt32) {
        if (bytes + sizeof(int32_t) > end)
          return nullptr;

        int32_t value = *(int32_t*) bytes;
        bytes += sizeof(int32_t);

        value_out.set_int32_value(value);
      } else if (type == Value::kString) {
        std::string value;

        bytes = ReadString(bytes, end, value);
        if (!bytes)
          return nullptr;

        value_out.set_string_value(value);
      }

      return bytes;
    }

    void Store(
      const char* bytes, size_t message_length,
      std::vector<char>& wire_out) {

      wire_out.insert(wire_out.end(), bytes, bytes + message_length);
    }

    void StoreString(const std::string& s, std::vector<char>& wire_out) {
      int32_t size = s.size();
      Store((const char*) &size, sizeof(int32_t), wire_out);

      Store(s.c_str(), s.size(), wire_out);
    }

    void StoreValue(const Value& value, std::vector<char>& wire_out) {
      if (value.type() != Value::kInt32 && value.type() != Value::kString)
        return;

      int8_t type = value.type();
      Store((const char*) &type, sizeof(int8_t), wire_out);

      if (type == Value::kInt32) {
        int32_t v = value.int32_value();
        Store((const char*) &v, sizeof(int32_t), wire_out);
      } else if (type == Value::kString) {
        StoreString(value.string_value(), wire_out);
      }
    }
  }

  bool Request::Read(const char* bytes, size_t message_length) {
    const char* end = bytes + message_length;

    if (bytes + sizeof(int8_t) > end)
      return false;

    int8_t type = *(int8_t*) bytes;
    bytes += sizeof(int8_t);

    if (type != kRequest)
      return false;

    if (bytes + sizeof(int32_t) > end)
      return false;

    int32_t id = *(int32_t*) bytes;
    bytes += sizeof(int32_t);

    std::string method;
    bytes = detail::ReadString(bytes, end, method);

    if (!bytes)
      return false;

    if (bytes + sizeof(int32_t) > end)
      return false;

    int32_t argc = *(int32_t*) bytes;
    bytes += sizeof(int32_t);

    std::vector<Value> args(argc);
    for (int32_t i = 0; i < argc; ++i) {
      bytes = detail::ReadValue(bytes, end, args[i]);
      if (!bytes)
        return false;
    }

    id_ = id;
    std::swap(method_, method);
    std::swap(args_, args);

    return true;
  }

  void Request::Store(std::vector<char>& wire_out) const {
    int8_t type = kRequest;
    detail::Store((const char*) &type, sizeof(int8_t), wire_out);

    detail::Store((const char*) &id_, sizeof(int32_t), wire_out);

    detail::StoreString(method_, wire_out);

    int32_t argc = (int32_t) args_.size();
    detail::Store((const char*) &argc, sizeof(int32_t), wire_out);

    for (int32_t i = 0; i < argc; ++i)
      detail::StoreValue(args_[i], wire_out);
  }

  bool Response::Read(const char* bytes, size_t message_length) {
    const char* end = bytes + message_length;

    if (bytes + sizeof(int32_t) > end)
      return false;

    int32_t id = *(int32_t*) bytes;
    bytes += sizeof(int32_t);

    Value result;
    bytes = detail::ReadValue(bytes, end, result);

    if (!bytes)
        return false;

    id_=  id;
    std::swap(result_, result);

    return true;
  }

  void Response::Store(std::vector<char>& wire_out) const {
    detail::Store((const char*) &id_, sizeof(int32_t), wire_out);

    detail::StoreValue(result_, wire_out);
  }
}
