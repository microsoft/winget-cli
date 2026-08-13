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

#ifndef AICLI_DISABLE_TEST_HOOKS
// Determines whether the current process is running as an administrator.
// Only used to relax security settings that a process below high integrity is not allowed to
// apply, so that the security E2E tests can run a medium integrity server process.
bool IsCurrentProcessAdmin();
#endif

std::string GetUserSID();

// Gets the SID of the current process user in binary form.
// The returned SID points into the returned buffer, which must outlive its use.
std::pair<std::unique_ptr<BYTE[]>, PSID> GetUserSidBinary();

// Returns the ncalrpc endpoint name that the manual activation server listens on.
std::string GetServerEndpointName();

// Creates or opens the mutex used to ensure a single manual activation server per user.
// The mutex is per-user and cannot be acquired below high integrity.
wil::unique_mutex CreateOrOpenServerMutex();

// Creates or opens the event used to signal that the manual activation server is ready.
// The event is per-user and cannot be signalled below high integrity.
wil::unique_event CreateOrOpenServerStartEvent();
