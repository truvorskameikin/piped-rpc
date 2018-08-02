#ifndef __RPC_PROTOCOL_H_
#define __RPC_PROTOCOL_H_

#include <stdint.h>
#include <memory>
#include <string>
#include <vector>

namespace rpc_protocol {
  class Value {
   public:
    enum Type {
      kTypeUnknown,
      kInt32,
      kString
    };

   public:
    Value() {
    }

    Value(int32_t v) {
      set_int32_value(v);
    }

    Value(const std::string& v) {
      set_string_value(v);
    }

    bool operator==(const Value& rhv) const {
      if (type_ == kTypeUnknown || rhv.type_ == kTypeUnknown)
        return false;
      if (type_ != rhv.type_)
        return false;

      if (type_ == kInt32)
        return int32_value_ == rhv.int32_value_;
      if (type_ == kString)
        return string_value_ == rhv.string_value_;

      return false;
    }

    Type type() const {
      return type_;
    }

    int32_t int32_value() const {
      return int32_value_;
    }

    void set_int32_value(int32_t value) {
      type_ = kInt32;
      int32_value_ = value;
    }

    const std::string& string_value() const {
      return string_value_;
    }

    void set_string_value(const std::string& value) {
      type_ = kString;
      string_value_ = value;
    }

   private:
    Type type_{kTypeUnknown};
    int32_t int32_value_{0};
    std::string string_value_;
  };

  const int8_t kRequest = 1;
  const int8_t kResponse = 2;

  namespace detail {
    const char* ReadString(
      const char* bytes, const char* end,
      std::string& string_out);

    const char* ReadValue(
      const char* bytes, const char* end,
      Value& value_out);

    void Store(
      const char* bytes, size_t message_length,
      std::vector<char>& wire_out);
  }

  class Request {
   public:
    int32_t id() const {
      return id_;
    }

    void set_id(int32_t id) {
      id_ = id;
    }

    const std::string& method() const {
      return method_;
    }

    void set_method(const std::string& method) {
      method_ = method;
    }

    void swap_method(std::string& method) {
      std::swap(method_, method);
    }

    const std::vector<Value>& args() const {
      return args_;
    }

    void add_arg(const Value& arg) {
      args_.push_back(arg);
    }

    void swap_args(std::vector<Value>& args) {
      std::swap(args_, args);
    }

    bool Read(const char* bytes, size_t message_length);

    void Store(std::vector<char>& wire_out) const;

   private:
    int32_t id_{0};
    std::string method_;
    std::vector<Value> args_;
  };

  class Response {
   public:
    int32_t id() const {
      return id_;
    }

    void set_id(int32_t id) {
      id_ = id;
    }

    const Value& result() const {
      return result_;
    }

    void set_result(const Value& result) {
      result_ = result;
    }

    void swap_result(Value& value) {
      std::swap(result_, value);
    }

    bool Read(const char* bytes, size_t message_length);

    void Store(std::vector<char>& wire_out) const;

   private:
    int32_t id_{0};
    Value result_;
  };

  class ClientTransport {
   public:
    virtual Response MakeRequest(const Request& request) = 0;
  };

  class ServerTransport {
   public:
    virtual rpc_protocol::Request Receive() = 0;
    virtual void Send(const rpc_protocol::Response& response) = 0;
  };

  class Server {
   public:
    virtual std::shared_ptr<ServerTransport> Accept() = 0;
  };
};

#endif
