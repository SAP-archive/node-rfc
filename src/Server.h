// SPDX-FileCopyrightText: 2014 SAP SE Srdjan Boskovic <srdjan.boskovic@sap.com>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef NodeRfc_Server_H
#define NodeRfc_Server_H

#include <uv.h>
#include <map>
#include "Client.h"

typedef struct
{
		RFC_FUNCTION_DESC_HANDLE func_desc_handle; 
		RFC_FUNCTION_HANDLE func_handle;
		RFC_ERROR_INFO *errorInfo;
		
	    node_rfc::RfmErrorPath errorPath;
  	    node_rfc::ClientOptionsStruct client_options;
		Napi::Reference<Napi::Array> paramNames;
		uint_t paramCount;
		
		uv_cond_t wait_js;
		uv_mutex_t wait_js_mutex; 
		bool js_running;
		uv_mutex_t js_running_mutex;
} ServerCallbackContainer;

void ServerCallJs(Napi::Env env, Napi::Function callback, std::nullptr_t *context, ServerCallbackContainer *data); // handles calling the JS callback
void ServerDoneCallback(const CallbackInfo& info); 																																 // called by the JS callback to signal completion of the callback (ABAP may now continue)
using ServerCallbackTsfn = Napi::TypedThreadSafeFunction<std::nullptr_t, ServerCallbackContainer, ServerCallJs>;

typedef struct _ServerFunctionStruct
{
    SAP_UC func_name[128];
    RFC_FUNCTION_DESC_HANDLE func_desc_handle = NULL;
    ServerCallbackTsfn threadSafeCallback;

    _ServerFunctionStruct()
    {
        func_name[0] = 0;
    }

    _ServerFunctionStruct(SAP_UC *name, RFC_FUNCTION_DESC_HANDLE desc_handle, ServerCallbackTsfn cb)
    {
        strcpyU(func_name, name);
        func_desc_handle = desc_handle;
        threadSafeCallback = cb;
    }

    _ServerFunctionStruct &operator=(_ServerFunctionStruct &src) // note: passed by copy
    {
        strcpyU(func_name, src.func_name);
        func_desc_handle = src.func_desc_handle;
        threadSafeCallback = src.threadSafeCallback;
        return *this;
    }

    ~_ServerFunctionStruct()
    {
        threadSafeCallback.Release();
    }
} ServerFunctionStruct;

typedef std::map<std::string, ServerFunctionStruct *> ServerFunctionsMap;

typedef struct
{
    napi_async_work work;
    napi_threadsafe_function tsfn;
} AddonData;

namespace node_rfc
{
    extern Napi::Env __env;

    class Server : public Napi::ObjectWrap<Server>
    {
    public:
        friend class StartAsync;
        friend class StopAsync;
        friend class GetFunctionDescAsync;
        static Napi::Object Init(Napi::Env env, Napi::Object exports);
        Server(const Napi::CallbackInfo &info);
        ~Server(void);
        ServerFunctionsMap serverFunctions;
        AddonData *addon_data;


    private:
        Napi::Value IdGetter(const Napi::CallbackInfo &info);
        Napi::Value AliveGetter(const Napi::CallbackInfo &info);
        Napi::Value ServerConnectionHandleGetter(const Napi::CallbackInfo &info);
        Napi::Value ClientConnectionHandleGetter(const Napi::CallbackInfo &info);

        Napi::Value Start(const Napi::CallbackInfo &info);
        Napi::Value Stop(const Napi::CallbackInfo &info);
        Napi::Value AddFunction(const Napi::CallbackInfo &info);
        Napi::Value RemoveFunction(const Napi::CallbackInfo &info);
        Napi::Value GetFunctionDescription(const Napi::CallbackInfo &info);

        //RFC_RC SAP_API metadataLookup(SAP_UC *func_name, RFC_ATTRIBUTES rfc_attributes, RFC_FUNCTION_DESC_HANDLE *func_handle);
        //RFC_RC SAP_API genericHandler(RFC_CONNECTION_HANDLE conn_handle, RFC_FUNCTION_HANDLE func_handle, RFC_ERROR_INFO *errorInfo);

        RFC_CONNECTION_HANDLE server_conn_handle;
        RFC_CONNECTION_HANDLE client_conn_handle;
        RFC_SERVER_HANDLE serverHandle;
        ConnectionParamsStruct server_params;
        ConnectionParamsStruct client_params;
        //ClientOptionsStruct client_options;

        Napi::ObjectReference serverParamsRef;
        Napi::ObjectReference clientParamsRef;
        Napi::ObjectReference clientOptionsRef;

        void init(Napi::Env env)
        {
            if (node_rfc::__env == NULL)
            {
                node_rfc::__env = env;
            };
            id = Server::_id++;

            server_conn_handle = NULL;
            client_conn_handle = NULL;
            serverHandle = NULL;

            uv_sem_init(&invocationMutex, 1);

            addon_data = (AddonData *)malloc(sizeof(*addon_data));
            addon_data->work = NULL;
        };

        static uint_t _id;
        uint_t id;

        void LockMutex();
        void UnlockMutex();
        uv_sem_t invocationMutex;
    };

} // namespace node_rfc

#endif
