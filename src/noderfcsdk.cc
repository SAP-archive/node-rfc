// Copyright 2014 SAP AG.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http: //www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
// either express or implied. See the License for the specific
// language governing permissions and limitations under the License.

#include "noderfcsdk.h"

namespace node_rfc
{
    Napi::Env __env = NULL;

    ////////////////////////////////////////////////////////////////////////////////
    // SAP to JS String
    ////////////////////////////////////////////////////////////////////////////////

    Napi::Value wrapString(SAP_UC *uc, int length)
    {
        RFC_RC rc;
        RFC_ERROR_INFO errorInfo;

        Napi::EscapableHandleScope scope(node_rfc::__env);

        if (length == -1)
        {
            length = strlenU((SAP_UTF16 *)uc);
        }
        if (length == 0)
        {
            return Napi::String::New(node_rfc::__env, "");
        }
        // try with 3 bytes per unicode character
        unsigned int utf8Size = length * 3;
        char *utf8 = (char *)malloc(utf8Size + 1);
        utf8[0] = '\0';
        unsigned int resultLen = 0;
        rc = RfcSAPUCToUTF8(uc, length, (RFC_BYTE *)utf8, &utf8Size, &resultLen, &errorInfo);

        if (rc != RFC_OK)
        {
            // not enough, try with 6
            free((char *)utf8);
            utf8Size = length * 6;
            utf8 = (char *)malloc(utf8Size + 1);
            utf8[0] = '\0';
            resultLen = 0;
            rc = RfcSAPUCToUTF8(uc, length, (RFC_BYTE *)utf8, &utf8Size, &resultLen, &errorInfo);
            if (rc != RFC_OK)
            {
                free((char *)utf8);
                char err[255];
                sprintf(err, "wrapString fatal error: length: %d utf8Size: %u resultLen: %u", length, utf8Size, resultLen);
                Napi::Error::Fatal(err, "node-rfc internal error");
            }
        }

        int i = strlen(utf8) - 1;
        while (i >= 0 && isspace(utf8[i]))
        {
            i--;
        }
        utf8[i + 1] = '\0';

        Napi::Value resultValue = Napi::String::New(node_rfc::__env, utf8);
        free((char *)utf8);
        return scope.Escape(resultValue);
    }

    ////////////////////////////////////////////////////////////////////////////////
    // RFC ERRORS
    ////////////////////////////////////////////////////////////////////////////////

    Napi::Value NodeRfcError(Napi::Value errorObj)
    {
        Napi::EscapableHandleScope scope(node_rfc::__env);
        return scope.Escape(errorObj);
    }

    Napi::Value RfcLibError(RFC_ERROR_INFO *errorInfo, bool alive)
    {
        Napi::EscapableHandleScope scope(node_rfc::__env);

        Napi::Object errorObj = Napi::Object::New(node_rfc::__env);
        (errorObj).Set(Napi::String::New(node_rfc::__env, "alive"), Napi::Boolean::New(node_rfc::__env, alive));
        (errorObj).Set(Napi::String::New(node_rfc::__env, "name"), "RfcLibError");
        (errorObj).Set(Napi::String::New(node_rfc::__env, "code"), Napi::Number::New(node_rfc::__env, errorInfo->code));
        (errorObj).Set(Napi::String::New(node_rfc::__env, "codeString"), wrapString((SAP_UC *)RfcGetRcAsString(errorInfo->code)));
        (errorObj).Set(Napi::String::New(node_rfc::__env, "key"), wrapString(errorInfo->key));
        (errorObj).Set(Napi::String::New(node_rfc::__env, "message"), wrapString(errorInfo->message));
        return scope.Escape(errorObj);
    }

    Napi::Value AbapError(RFC_ERROR_INFO *errorInfo, bool alive)
    {
        Napi::EscapableHandleScope scope(node_rfc::__env);

        Napi::Object errorObj = Napi::Object::New(node_rfc::__env);
        (errorObj).Set(Napi::String::New(node_rfc::__env, "alive"), Napi::Boolean::New(node_rfc::__env, alive));
        (errorObj).Set(Napi::String::New(node_rfc::__env, "name"), "ABAPError");
        (errorObj).Set(Napi::String::New(node_rfc::__env, "code"), Napi::Number::New(node_rfc::__env, errorInfo->code));
        (errorObj).Set(Napi::String::New(node_rfc::__env, "codeString"), wrapString((SAP_UC *)RfcGetRcAsString(errorInfo->code)));
        (errorObj).Set(Napi::String::New(node_rfc::__env, "key"), wrapString(errorInfo->key));
        (errorObj).Set(Napi::String::New(node_rfc::__env, "message"), wrapString(errorInfo->message));
        (errorObj).Set(Napi::String::New(node_rfc::__env, "abapMsgClass"), wrapString(errorInfo->abapMsgClass));
        (errorObj).Set(Napi::String::New(node_rfc::__env, "abapMsgType"), wrapString(errorInfo->abapMsgType));
        (errorObj).Set(Napi::String::New(node_rfc::__env, "abapMsgNumber"), wrapString(errorInfo->abapMsgNumber));
        (errorObj).Set(Napi::String::New(node_rfc::__env, "abapMsgV1"), wrapString(errorInfo->abapMsgV1));
        (errorObj).Set(Napi::String::New(node_rfc::__env, "abapMsgV2"), wrapString(errorInfo->abapMsgV2));
        (errorObj).Set(Napi::String::New(node_rfc::__env, "abapMsgV3"), wrapString(errorInfo->abapMsgV3));
        (errorObj).Set(Napi::String::New(node_rfc::__env, "abapMsgV4"), wrapString(errorInfo->abapMsgV4));

        return scope.Escape(errorObj);
    }

    Napi::Value wrapError(RFC_ERROR_INFO *errorInfo, bool alive)
    {
        Napi::EscapableHandleScope scope(node_rfc::__env);

        char cBuf[256];

        switch (errorInfo->group)
        {
        case OK: // 0: should never happen
            Napi::Error::Fatal("Error handling invoked with the RFC error group OK", "node-rfc internal error");
            break;

        case LOGON_FAILURE:            // 3: Error message raised when logon fails
        case COMMUNICATION_FAILURE:    // 4: Problems with the network connection (or backend broke down and killed the connection)
        case EXTERNAL_RUNTIME_FAILURE: // 5: Problems in the RFC runtime of the external program (i.e "this" library)
            return scope.Escape(RfcLibError(errorInfo, alive));
            break;

        case ABAP_APPLICATION_FAILURE:       // 1: ABAP Exception raised in ABAP function modules
        case ABAP_RUNTIME_FAILURE:           // 2: ABAP Message raised in ABAP function modules or in ABAP runtime of the backend (e.g Kernel)
        case EXTERNAL_APPLICATION_FAILURE:   // 6: Problems in the external program (e.g in the external server implementation)
        case EXTERNAL_AUTHORIZATION_FAILURE: // 7: Problems raised in the authorization check handler provided by the external server implementation
            return scope.Escape(AbapError(errorInfo, alive));
            break;
        }

        // unknown error group
        sprintf(cBuf, "Unknown error group: %u", errorInfo->group);
        Napi::Error::Fatal(cBuf, "node-rfc internal error");
    }
} // namespace node_rfc
