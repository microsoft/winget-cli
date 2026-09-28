// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#pragma warning( push )
#pragma warning ( disable : 6001 6388 6553)
#include <wil/resource.h>
#include <wil/token_helpers.h>
#pragma warning( pop )
#include <string>

unsigned char* GetUCharString(const std::string& str);

#ifndef AICLI_DISABLE_TEST_HOOKS
// Determines whether the current process is running as an administrator.
// Only used to relax security settings that a process below high integrity is not allowed to
// apply, so that the security E2E tests can run a medium integrity server process.
bool IsCurrentProcessAdmin();
#endif

std::string GetUserSID();

// Gets the user of the current process token; the SID is at User.Sid.
// The returned value owns the SID, so it must outlive any use of that pointer.
wil::unique_tokeninfo_ptr<TOKEN_USER> GetBinaryUserSID();

// Returns the ncalrpc endpoint name that the manual activation server listens on.
std::string GetServerEndpointName();

// The single-instance server mutex, together with the private namespace that contains it.
// Both have to be held for the lifetime of the server: once the namespace handle is closed the
// namespace can no longer be opened, which is how a second instance finds the mutex in order to
// discover that a server is already running.
struct ServerMutex
{
    wil::unique_private_namespace_close PrivateNamespace;
    wil::unique_mutex Mutex;

    HANDLE get() const { return Mutex.get(); }
};

// Creates or opens the mutex used to ensure a single manual activation server per user.
// The mutex lives in a private namespace that only a process running as this user at high
// integrity can reach, and additionally cannot be acquired below high integrity.
// Throws E_ACCESSDENIED if the mutex already existed and does not enforce that.
ServerMutex CreateOrOpenServerMutex();

#ifndef AICLI_DISABLE_TEST_HOOKS
// Attempts to create or open the server's private namespace using the high integrity boundary
// that an elevated server uses, rather than the boundary that this process is entitled to.
// This is the squatting attempt that the namespace exists to defeat, so the security E2E tests
// use it to check that a lower integrity process cannot reach the mutex by name.
// Returns a null handle if the namespace could not be entered.
wil::unique_private_namespace_close TryEnterHighIntegrityServerNamespace();
#endif

// Creates or opens the event used to signal that the manual activation server is ready.
// The event is per-user and cannot be signalled below high integrity.
// Throws E_ACCESSDENIED if the event already existed and does not enforce that.
wil::unique_event CreateOrOpenServerStartEvent();
