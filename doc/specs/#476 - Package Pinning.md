---
author: Yao Sun @yao-msft
created on: 2022-10-12
last updated: 2022-10-12
issue id: 476
---

# Package Pinning

For [#476](https://github.com/microsoft/winget-cli/issues/476)

## Abstract

This spec describes the functionality and high level implementation design of Package Pinning feature.

## Inspiration

This is inspired by functionalities in other package managers, as well as community feedback.
- Packages may introduce breaking changes that users may not want integrate into their workflow quite yet.
- Packages may update themselves so that it will be duplicate effort for winget to try to update them.
- User may want to maintain some of the packages through other channels outside of winget, or prefer one source over others within winget.
- User may want some of the packages to stay in some major versions but allow minor version changes during upgrade.

## Solution Design

[comment]: # Outline the design of the solution. Feel free to include ASCII-art diagrams, etc.

## UI/UX Design

[comment]: # What will this fix/feature look like? How will it affect the end user?

## Capabilities

[comment]: # Discuss how the proposed fixes/features impact the following key considerations:

### Accessibility

[comment]: # How will the proposed change impact accessibility for users of screen readers, assistive input devices, etc.

### Security

[comment]: # How will the proposed change impact security?

### Reliability

[comment]: # Will the proposed change improve reliability? If not, why make the change?

### Compatibility

[comment]: # Will the proposed change break existing code/behaviors? If so, how, and is the breaking change "worth it"?

### Performance, Power, and Efficiency

## Potential Issues

[comment]: # What are some of the things that might cause problems with the fixes/features proposed? Consider how the user might be negatively impacted.

## Future considerations

[comment]: # What are some of the things that the fixes/features might unlock in the future? Does the implementation of this spec enable scenarios?

## Resources

[comment]: # Be sure to add links to references, resources, footnotes, etc.
