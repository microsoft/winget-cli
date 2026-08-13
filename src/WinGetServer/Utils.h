// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#pragma warning( push )
#pragma warning ( disable : 6001 6388 6553)
#include <wil/resource.h>
#pragma warning( pop )
#include <memory>
#include <string>
#include <utility>

unsigned char* GetUCharString(const std::string& str);

std::string GetUserSID();

// Gets the SID of the current process user in binary form.
// The returned SID points into the returned buffer, which must outlive its use.
std::pair<std::unique_ptr<BYTE[]>, PSID> GetUserSidBinary();

// Returns the ncalrpc endpoint name that the manual activation server listens on.
std::string GetServerEndpointName();

wil::unique_event CreateOrOpenServerStartEvent();
