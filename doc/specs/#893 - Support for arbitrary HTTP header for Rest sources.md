---
author: Ashwini Patil @ashpatil
created on: 2021-08-11
last updated: 2021-08-11
issue id: 893
---

# Support for arbitrary HTTP header for Rest sources

For [#893](https://github.com/microsoft/winget-cli/issues/893).

## Abstract

There is a need for some REST sources to support custom behaviors when interacting with Windows Package Manager Client. The client should be able to pass data to a REST source via an HTTP header provided by the user. This could be used for any appropriate scenario. One example might be passing data to a source to indicate a specific behavior for the source to act on. For example, if you want the source to return an invalid response to test client behavior.
The optional HTTP header can be passed in as a command-line argument or configured as a setting. This will be passed as a header in every request to the Rest sources that are configured.

If no Rest sources are configured the header will be ignored and a warning message will be displayed.

## Inspiration

An ability for the REST source to define custom behavior based on data provided in the HTTP header.

## Solution Design

An optional command-line argument named `--header` will be exposed for the commands that interact with Rest source.
A new settings block with source name and header will be added. This can be used as an alternative to the command-line parameter and will provide an advantage of configuring different header information for different Rest sources.

If both command-line argument and setting are found for the header, the command-line argument will take precedence. If no particular Rest source is specified in the command, the same header will be sent to all the configured Rest sources.

If the command-line argument or a setting are provided for the header and there are no Rest sources configured, the header will be ignored and an appropriate message will be displayed. The same behaviour applies to a case where a source other than Rest Source is specified using `-s` command.

If Rest sources are configured and the header is provided, the client will send it as a value of HTTP header named `Windows-Package-Manager`. The source may use it any way the source sees fit.

## UI/UX Design

An optional command-line argument named `--header` will be exposed on the commands `winget source add, search, install, upgrade, import and show`.

## Settings

Users will have an option to add the source name and header value to the settings file. The settings file can be opened via the settings command.

```
  "sourceData": [
          {
            "sourceName": "RestSourceName",
            "header": "Custom header for source"
          }
      ]
  
```

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
