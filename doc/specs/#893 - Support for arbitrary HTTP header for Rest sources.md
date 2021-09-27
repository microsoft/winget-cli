---
author: Ashwini Patil @ashpatil
created on: 2021-08-11
last updated: 2021-09-01
issue id: 893
---

# Support for arbitrary HTTP header for Rest sources

For [#893](https://github.com/microsoft/winget-cli/issues/893).

## Abstract

There is a need for some REST sources to support custom behaviors when interacting with Windows Package Manager Client. The client should be able to pass data to a REST source via an HTTP header provided by the user. This could be used for any appropriate scenario. One example might be passing data to a source to indicate a specific behavior for the source to act on. For example, if you want the source to return an invalid response to test client behavior.
The optional HTTP header can be passed in as a command-line argument with winget commands and a specific Rest source option.

If the source specified is not a Rest source, the header will be ignored and a warning message will be displayed.

## Inspiration

An ability for the REST source to define custom behavior based on data provided in the HTTP header.

## Solution Design

An optional command-line argument named `--header` will be exposed for the commands that interact with Rest source. The header argument should be accompanied with a source argument that specifies which source should the custom header should be sent to. If the source specified is something other than a Rest source in the command, the header will be ignored and a warning message will be displayed. If no source is specified, an error message will be displayed with more information. The header can have a maximum length of 1024 characters. If the length of the header input exceeds this, an error message will be displayed with information.

The client will send the header as a value of HTTP header named `Windows-Package-Manager`. The source may use it any way the source sees fit.

## UI/UX Design

An optional command-line argument named `--header` will be exposed on the winget commands `source add, search, install, uninstall, list, upgrade and show`. Support for `import/export` commands with header will be added when we get more information on how custom header will be used in those commands.

### Accessibility

This should not change impact accessibility for users of screen readers, assistive input devices, etc.

### Security

### Reliability

This should not impact reliability.

### Compatibility

### Performance, Power, and Efficiency

## Potential Issues

## Future considerations

## Resources

Issue [#893](https://github.com/microsoft/winget-cli/issues/893) Add support for an arbitrary HTTP header value in REST API.
