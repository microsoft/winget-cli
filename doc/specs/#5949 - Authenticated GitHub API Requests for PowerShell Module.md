---
author: Melvin Wang @wmmc88
created on: 2026-02-09
last updated: 2026-02-26
issue id: 5949
---

# Authenticated GitHub API Requests for PowerShell Module

For [#5949](https://github.com/microsoft/winget-cli/issues/5949).

## Abstract

This spec describes adding support for authenticated GitHub API requests in the WinGet PowerShell module. The module's `GitHubClient` helper will automatically detect `GH_TOKEN` or `GITHUB_TOKEN` environment variables and use them to authenticate Octokit API calls, significantly increasing the GitHub API rate limit.

## Inspiration

Users running `Repair-WinGetPackageManager` in GitHub Actions pipelines hit unauthenticated rate limits (60 requests/hour). Authenticated requests allow 5,000 requests/hour. The GitHub CLI (`gh`) already uses `GH_TOKEN` and `GITHUB_TOKEN` for the same purpose, and GitHub Actions automatically provides `GITHUB_TOKEN`.

See: https://github.com/microsoft/windows-drivers-rs/actions/runs/20531244312/job/58982795057#step:3:43

## Solution Design

The `GitHubClient` class in `Microsoft.WinGet.Client.Engine` is updated to:

1. Read all known token environment variables (`GH_TOKEN`, `GITHUB_TOKEN`) on construction.
2. Log the presence or absence of each token via `StreamType.Verbose`.
3. Select the token to use based on precedence (`GH_TOKEN` > `GITHUB_TOKEN`), matching GitHub CLI behavior.
4. Log which token source is being used, or that no token was found.
5. Set `Octokit.GitHubClient.Credentials` if a token is available.

Token resolution is extracted into a static `ResolveGitHubToken` method for testability.

### Token Precedence

`GH_TOKEN` takes precedence over `GITHUB_TOKEN`, matching the [GitHub CLI convention](https://cli.github.com/manual/gh_help_environment). This is because `GH_TOKEN` is explicitly set by users, while `GITHUB_TOKEN` is automatically provided by GitHub Actions and may have more restricted permissions.

### Logging

All logging uses `StreamType.Verbose` via the existing `PowerShellCmdlet.Write` pattern, visible when users pass `-Verbose` to cmdlets. Example output:

```
VERBOSE: GH_TOKEN environment variable: not found
VERBOSE: GITHUB_TOKEN environment variable: found
VERBOSE: Using authenticated GitHub API requests via GITHUB_TOKEN environment variable.
```

### Files Changed

- `src/PowerShell/Microsoft.WinGet.Client.Engine/Helpers/GitHubClient.cs` — Token resolution logic and logging.
- `src/PowerShell/Microsoft.WinGet.Client.Engine/Properties/AssemblyInfo.cs` — `InternalsVisibleTo` for unit tests.
- `src/PowerShell/Microsoft.WinGet.Client.Engine/Commands/WinGetPackageManagerCommand.cs` — Pass `PowerShellCmdlet` to `GitHubClient` constructor.
- `src/PowerShell/Microsoft.WinGet.Client.Engine/Helpers/AppxModuleHelper.cs` — Pass `PowerShellCmdlet` to `GitHubClient` constructor.
- `src/PowerShell/Microsoft.WinGet.UnitTests/GitHubClientTests.cs` — Unit tests for token resolution.

## UI/UX Design

No new command-line arguments or user-facing changes. The feature is automatic: if `GH_TOKEN` or `GITHUB_TOKEN` is set in the environment, authenticated requests are used. Users can see which token is being used by running any repair/assert cmdlet with `-Verbose`.

### Accessibility

No impact on accessibility.

### Security

- Tokens are never logged or written to output; only the environment variable name is logged.
- Token values are read from environment variables which are a standard secure mechanism for passing secrets in CI/CD environments.
- Whitespace-only token values are treated as unset to prevent accidental empty-credential authentication.

### Reliability

Improves reliability by reducing GitHub API rate limit failures in CI/CD pipelines. Unauthenticated requests are still used as a fallback when no tokens are set.

### Compatibility

Fully backward compatible. When no token environment variables are set, behavior is identical to the previous implementation.

### Performance, Power, and Efficiency

No measurable impact. A single environment variable read per `GitHubClient` construction.

## Potential Issues

- If a user has an expired or revoked token in `GH_TOKEN`/`GITHUB_TOKEN`, API calls will fail with a 401 rather than falling back to unauthenticated. This matches GitHub CLI behavior and is the expected outcome.

## Future considerations

- Support for additional token sources (e.g., `gh auth token` integration, Windows Credential Manager).
- Applying authenticated requests to other parts of the WinGet client beyond the PowerShell module.

## Resources

- [GitHub API rate limits](https://docs.github.com/en/rest/using-the-rest-api/rate-limits-for-the-rest-api)
- [GitHub CLI environment variables](https://cli.github.com/manual/gh_help_environment)
- [Octokit.NET authenticated access](https://octokitnet.readthedocs.io/en/latest/getting-started/#authenticated-access)
- [Issue #5949](https://github.com/microsoft/winget-cli/issues/5949)
