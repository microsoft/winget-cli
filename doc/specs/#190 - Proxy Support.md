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
This functionality will first need to be enabled through an admin setting, similar to local manifests or hash override.

An option to set a default proxy to use on every flow will be added, but it will require administrator permissions to be set.

New Group Policy will also be added for IT admins to control the use of proxies.
The policies will be similar to those we already have for sources, so that a specific proxy can be required or only a predefined set of proxies can be allowed.

Proxies will not be used for the configuration features for now.
Proxy settings in winget will not affect the behavior of installers themselves, so an installation may still generate traffic outside of the proxy.

The proxy will be used to download the installers and access the sources.

Since the APIs used for Delivery Optimization and MSIX deployment do not provide a way to specify a custom proxy, if a proxy is specified, we will change the use of those APIs to accomodate the proxy.
For Delivery Optimization, a proxy will force use of WinINet instead.
For MSIX deployment, the packages will be fully downloaded before deploying; this will require more network traffic than a streaming install of only the required bits.

## UI/UX Design

We will add a command line argument taking the URI to the proxy.
A separate argument will be available to disable the use of proxy if there is a default set.
Both of these arguments will be disabled by default and require admin privileges to enable.

```
> winget settings --enable ProxyCommandLineArgument
> winget install Contoso.App --proxy https://127.0.0.1:2345
> winget install Contoso.App --no-proxy
```

To configure the default proxy, new `set` and `reset` subcommands will be added to the `settings` command.
This will require admin privileges and does not require `ProxyCommandLineArgument` to be enabled.

```
> winget settings set DefaultProxy https://127.0.0.1:2345
> winget settings reset DefaultProxy
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
* (New) A Group Policy will be available to block the use of proxies, require the use of a specific proxy, or limit them to an approved list.
* Pre-indexed sources need to be signed, and the publisher is required to match during source update.
  When initially adding the source, administrator privileges are already required to limit misuse.
* Pre-indexed sources include manifest hashes in the local database, to ensure that the manifest downloaded later is as expected.
* For the Microsoft Store source, we use certificate pinning to ensure we are talking to the right server.
* When communicating with REST sources, the certificate used by the source for HTTPS needs to match the domain.
* Manifests include a hash of the installer that is validated before executing it.
  The ability to ignore installer hash mismatches is disabled by default, and enabling it requires administrator privileges.

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
* Add the ability for admins to set multiple allowed proxies that a user can use
* Add the ability to specify a different default proxy for each source
* Use proxies with Delivery Optimization. This requires changes to the Delivery Optimization APIs.
