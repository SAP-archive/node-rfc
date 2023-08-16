// SPDX-FileCopyrightText: 2014 SAP SE Srdjan Boskovic <srdjan.boskovic@sap.com>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef NodeRfc_Server_H
#define NodeRfc_Server_H

#include <condition_variable>
#include <thread>
#include <unordered_map>
#include "Client.h"

namespace node_rfc {

extern Napi::Env __env;
extern Log _log;

typedef struct _ServerOptions {
  logLevel log_severity = logLevel::none;

} ServerOptions;

class ServerRequestBaton;

RFC_RC SAP_API metadataLookup(SAP_UC const* func_name,
                              RFC_ATTRIBUTES rfc_attributes,
                              RFC_FUNCTION_DESC_HANDLE* func_handle);

RFC_RC SAP_API genericRequestHandler(RFC_CONNECTION_HANDLE conn_handle,
                                     RFC_FUNCTION_HANDLE func_handle,
                                     RFC_ERROR_INFO* errorInfo);

class Server : public Napi::ObjectWrap<Server> {
 public:
  friend class StartAsync;
  friend class StopAsync;
  friend class GetFunctionDescAsync;
  friend class ServerFunction;
  friend class ServerRequestBaton;

  std::string get_request_id() {
    return std::to_string(id) + ":" + std::to_string(Server::request_id);
  }
  std::string next_request_id() {
    return std::to_string(id) + ":" + std::to_string(++Server::request_id);
  }
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  // cppcheck-suppress noExplicitConstructor
  Server(const Napi::CallbackInfo& info);
  ~Server(void);

  ClientOptionsStruct client_options;
  std::thread server_thread;
  const std::string log_id() const { return "Server " + std::to_string(id); }

 private:
  void _stop();
  void _start(RFC_ERROR_INFO* errorInfo);

  Napi::Env env = nullptr;

  Napi::Value IdGetter(const Napi::CallbackInfo& info);
  Napi::Value AliveGetter(const Napi::CallbackInfo& info);
  Napi::Value ServerConnectionHandleGetter(const Napi::CallbackInfo& info);
  Napi::Value ClientConnectionHandleGetter(const Napi::CallbackInfo& info);

  Napi::Value Start(const Napi::CallbackInfo& info);
  Napi::Value Stop(const Napi::CallbackInfo& info);
  Napi::Value AddFunction(const Napi::CallbackInfo& info);
  Napi::Value RemoveFunction(const Napi::CallbackInfo& info);
  Napi::Value GetFunctionDescription(const Napi::CallbackInfo& info);

  RFC_CONNECTION_HANDLE server_conn_handle;
  RFC_CONNECTION_HANDLE client_conn_handle;
  RFC_SERVER_HANDLE serverHandle;
  ConnectionParamsStruct server_params = ConnectionParamsStruct(0, nullptr);
  ConnectionParamsStruct client_params = ConnectionParamsStruct(0, nullptr);
  ServerOptions server_options = ServerOptions();

  Napi::ObjectReference serverConfigurationRef;

  void init(Napi::Env env) {
    id = Server::_id++;
    request_id = 0;
    this->env = env;

    server_conn_handle = nullptr;
    client_conn_handle = nullptr;
    serverHandle = nullptr;

    // uv_sem_init(&invocationMutex, 1);
  };

  static uint_t _id;
  uint_t id;
  static uint_t request_id;

  void LockMutex();
  void UnlockMutex();
  // uv_sem_t invocationMutex;
};

}  // namespace node_rfc

#endif
