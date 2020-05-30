---
author: Demitrius Nelon @denelon
created on: <2020-05-28>
last updated: <2020-05-28>
issue id: 292
---

# Package matching bug fix

For [#292](https://github.com/microsoft/winget-cli/issues/292)

## Abstract

The winget.exe client attempts to be generous with the `search` command, but is a bit too generous with `install`. The *id*
should be the unique key to identifying a package (other than the package version). It should also be case insensitve from the perspective of command execution, but it should be case sensitive in terms of the displayed value. 

If a manifest was created with the *id* "Git.Git" then that is what would be displayed in the client output.
Any combination of case in the `install` command should match.

If multiple versions of the package have been created, and they differ in the casing for the *id* then the most recent version
of the manifest should be used for displaying the characters in the search output.

The latest version of a package is also the version that should be displayed during `search`, and subsequently installed.

## Inspiration

`winget install git.git` should work.

## Solution Design

Installing a package by using the *Id* as the package name should not require disambiguation.
The package *Id* should perform a case insensitve match. If the given *Id* is exact, it should not
be confused with a longer *Id*
`winget install git.git` should install that package
The `git.gitLFS` should not cause ambiguity

If the value passed as a package *Id* is a substring, or there is still ambiguity, the help text should provide an example.

## UI/UX Design

Executing `winget install git.git` will install the Git package.
Executing `winget install git.g` will provode additional guidance:
Multiple apps found matching input criteria. Please refine the input.
Try providing the Id: winget install git.git

For the example above, the help could default to the Id for the top listed package from the search result.

## Capabilities

This should help reduce the friction for users trying to install a package when there is ambiguity.

### Accessibility

This may have an impact on users with screen readers. The goal is to provide additional context for this scenario.

### Security

This should not introduce any _new_ security concerns.

### Reliability

This is not expected to impact reliability.

### Compatibility

This changes existing behavior with `install`, but is expected to be an improvement. There are no known unintended consequences.

### Performance, Power, and Efficiency

## Potential Issues

Users may not realize another package exists with a longer name, and may install a program other than what they intended.
If "git.git" and "git.gity" both existed and the user were to press enter before typing the final "y", the git.git package would
be installed rather than what they intended.

## Future considerations

There are changes to how results should displayed to reduce the likelihood of a user mistakenly assuming the "Name" is a key value
for a package. The client commands should be case insensitive, but the display should still be case sensitive to support ease of
reading. Long names can be easier to understand when presented in camel case, pascal case, or with a branded letter casing.
The client also needs a mechanism to display all of the available versions of a package.

## Resources

N/A
