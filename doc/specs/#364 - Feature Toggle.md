---
author: John McPherson @JohnMcPMS
created on: 2020-06-04
last updated: 2020-06-04
issue id: 364
---

# Feature Toggle

For [#364](https://github.com/microsoft/winget-cli/issues/364)

## Abstract

As features are implemented within winget, they may cause disruption to users oblivious to the fact that they are in progress. In order to allow work to be done in master, and distributed to early adopters for their feedback, this spec suggests that settings should be used to control "experimental" features. We use this term [as others have](https://github.com/PowerShell/PowerShell-RFC/blob/master/5-Final/RFC0029-Support-Experimental-Features.md) to mean work in progress that is not yet ready for release to the general audience.

## Inspiration

We realize that we need to be able to:
1. deliver work in progress to users so that we can receive feedback
2. work in master without creating multiple, divergent branches
3. not disrupt users who do not wish to investigate unfinished features

Thank you to @megamorf for the link to the PowerShell Experimental Features RFC.

## Solution Design

A new settings block will be added to the settings file for experimental features to be toggled. By default, all experimental features will be disabled.

A hidden command will be added to the root:
```
>winget features
The following experimental features are in progress.
They can be configured through the settings file (winget settings).

Feature  Status   Link
--------------------------------------------------------------------
Example1 Enabled  https://github.com/microsoft/winget-cli/issues/364
Example2 Disabled https://github.com/microsoft/winget-cli/issues/396
```
When run, this command will give the status of each feature (enabled/disabled), as well as a link to the Issue or Spec.

Internally, a single flag enum will be used to reference experimental features. All command line parsing objects will have a field added for the feature(s) to which they belong. Any internal behavioral changes will be made based on a check using this enum. In this way, transitioning a feature either to release or remove can be done by finding by a single identifier.

A feature is released by removing the feature enum value in code, and all related references. The value in settings will become ignored, and we can keep a list in a comment to prevent future reuse of settings names.

## Settings File

To enable experimental features a user will need to modify the settings file. The settings file can be opened via the settings command.

```
  "experimentalFeatures": {
      "example1": true,
      "example2": false,
      "example3": true
  }
```

## UI/UX Design

Experimental features can impact the winget user interface in 2 ways; new commands and options. Both will be added to the parse tree, but hidden from help view if disabled. When enabled, they will be labeled as experimental to reiterate this to the user. If a user attempts to use an experimental command or option that is not enabled, a special error will be presented to indicate that this feature is experimental and must be enabled via settings.

If the feature needs settings after release, they should be added as part of the feature work, and can be tagged with the enum value as well.

## Capabilities



### Accessibility

This should have no direct impact on accessibility.

### Security

There should be no security impact directly, although we must remember that any medium IL process will be able to enable any feature by writing to the settings file. But any experimental feature should be created with the intention of becoming released, and so should have it's own security consideration.

### Reliability

One of the goals is to increase reliability for unaware users, so that they do not accidentally stumble into an incomplete feature.

### Compatibility

No breaking changes to existing behavior.

### Performance, Power, and Efficiency

## Potential Issues

No known issues.

## Future considerations

This feature enables all larger features in the future to have a phased rollout.

## Resources

[PowerShell Experimental Features RFC](https://github.com/PowerShell/PowerShell-RFC/blob/master/5-Final/RFC0029-Support-Experimental-Features.md)
