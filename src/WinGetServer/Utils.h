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

// The events used to signal that the manual activation server is ready.
//
// There are two of them because the name of the public one is reachable by any process running
// as this user, and so can be created by one of them before the server or the client gets to it.
// The security attributes passed to a create call are ignored when the name already exists, so
// whoever creates it first chooses the security of the object; a squatter that does so can
// signal it early and defeat the wait for the server to become ready. Refusing such an event
// outright would turn that into a denial of service, so a squatted public event is instead
// ignored in favour of the private one, whose name cannot be reached at all from outside the
// server's private namespace.
//
// The public event is kept only for compatibility with clients that shipped before the private
// one existed and know no other name. Once both sides are updated the private event carries the
// signal on its own.
struct ServerStartEvents
{
    // The event in the session object namespace, which previously shipped clients wait on.
    // Empty if it already existed and does not enforce what we asked for.
    wil::unique_event PublicEvent;

    // The event inside the server's private namespace, which cannot be squatted.
    wil::unique_event PrivateEvent;

    // Signals every event that is held.
    void SignalAll() const;

    // Waits for any one of the events that is held to be signalled, and returns whether that
    // happened before the timeout. Returns false if no events are held.
    bool WaitForAny(DWORD timeoutMilliseconds) const;

    // Releases every event that is held.
    void Reset();
};

// The named objects that coordinate the manual activation server, together with the private
// namespace that contains the protected ones.
//
// The namespace has to be held for as long as the objects in it are needed: once the last handle
// to it is closed the namespace can no longer be opened, which is how a second server instance
// finds the mutex in order to discover that a server is already running.
struct ServerSynchronization
{
    wil::unique_private_namespace_close PrivateNamespace;

    // Ensures a single manual activation server per user. Only the server waits on it; the
    // client merely keeps it alive, as holding a handle does not take ownership.
    wil::unique_mutex Mutex;

    ServerStartEvents StartEvents;
};

// Creates, or opens if a server is already running, the server's named synchronization objects.
// The protected ones live in a private namespace that only a process running as this user at
// high integrity can reach, and additionally cannot be used below high integrity.
// Throws E_ACCESSDENIED if one of those objects already existed and does not enforce that.
ServerSynchronization CreateOrOpenServerSynchronization(bool openMutex = false);

#ifndef AICLI_DISABLE_TEST_HOOKS
// Attempts to create or open the server's private namespace using the high integrity boundary
// that an elevated server uses, rather than the boundary that this process is entitled to.
// This is the squatting attempt that the namespace exists to defeat, so the security E2E tests
// use it to check that a lower integrity process cannot reach the protected objects by name.
// Returns a null handle if the namespace could not be entered.
wil::unique_private_namespace_close TryEnterHighIntegrityServerNamespace();
#endif
