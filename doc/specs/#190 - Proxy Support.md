---
author: Flor Chacon @florelis
created on: 2024-02-07
last updated: 2024-02-07
issue id: 190
---

# Proxy support

For [#190](https://github.com/microsoft/winget-cli/issues/190)

## Abstract

This spec describes a feature to specify a proxy for winget to use when connecting to the internet.

## Solution Design

A new command line argument will be added to specify a proxy to use during a particular invocation of winget.

An option to set a default proxy to use on every flow will be added, but it will be gated to require administrator permissions.

New Group Policy will also be added for IT admins to control the use of proxies.
The policies will be similar to those we already have for sources, so that a specific proxy can be required or only a predefined set of proxies can be allowed.

Proxies will not be used for the configuration features for now.

If a proxy is configured, it will be used for accessing sources and downloading installers.
Note that it will not affect the behavior of installers themselves, so an installation may still generate traffic outside of the proxy.

## UI/UX Design

We will add a command line argument taking the URI to the proxy.
A separate parameter will be available to disable the use of proxy if there is a default set.

```
> winget install Contoso.App --proxy https://127.0.0.1:2345
> winget install Contoso.App --no-proxy
```

To configure the default proxy, a new `proxy` subcommand will be added to the `settings` command, with options to `set` and `reset` the default.

```
> winget settings proxy set https://127.0.0.1:2345
> winget settings proxy reset
```

The current default proxy will be added to the output `winget --info`.

## Capabilities

### Accessibility

This should have no direct impact on accessibility.

### Security

There is a possibility of an attacker using a malicious proxy to tamper with the data received from the source, or with the contents of the installer file.
This is not much different from the risks of using a public network.
The following mitigating factors will be in place:
* (New) The ability to set a default proxy will be restricted to administrators, to prevent attackers from adding a proxy without the user realizing.
* (New) A Group Policy will be available to block the use of proxies, or limit it to an approved list.
* Pre-indexed sources need to be signed, and the publisher is required to match during source update.
  When initially adding the source, administrator privileges are already required to limit misuse.
* Pre-indexed sources include manifest hashes in the local database, to ensure that the manifest downloaded later is as expected.
* For the Microsoft Store source, we use certificate pinning to ensure we are talking to the right server.
* Manifests include a hash of the installer that is validated before executing it.
  The ability to ignore installer hash mismatches requires administrator privileges.

### Compatibility

No breaking changes to existing behavior.

### Performance, Power, and Efficiency

There should not be any notable performance changes.

## Potential Issues

A faulty or misconfigured proxy could impact most of winget's functionality, but it can be worked around by disabling the use of the proxy.

## Future considerations

Things we may want to consider in the future:
* Extend support for proxies to the Configuration feature
* Add proxy support to the COM API
* Add support for proxies that require authentication
