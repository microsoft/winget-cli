# Pipeline templates

These templates hold the Azure Pipelines jobs that build and test winget-cli. `azure-pipelines.yml`
at the repository root consumes them, and so can a pipeline in another repository that vendors
winget-cli as a subtree.

## Layout

| File | Kind | Purpose |
| --- | --- | --- |
| `jobs-build.yml` | jobs | Builds the solution and publishes the `Build.*` artifacts |
| `jobs-test.yml` | jobs | Unit, E2E, and configuration tests against those artifacts |
| `jobs-powershell-module.yml` | jobs | Builds and Pester-tests the PowerShell modules |
| `e2e-setup.yml` | steps | Test certificates, localhost web server, local PS repository |
| `e2e-test.template.yml` | steps | One E2E test pass, with optional COM tracing |

The two step templates are used by the job templates; a consumer does not reference them directly.

## `sourceRoot`

`jobs-build.yml` and `e2e-test.template.yml` take a `sourceRoot` parameter that points at the root
of the winget-cli sources. It defaults to `$(Build.SourcesDirectory)`, so a pipeline at the
repository root needs to pass nothing. (`e2e-setup.yml` predates this and spells the same idea
`sourceDir`, which is required rather than defaulted.)

```yaml
- template: templates/jobs-build.yml
  parameters:
    sourceRoot: $(Build.SourcesDirectory)\subtree
```

`jobs-test.yml` and `jobs-powershell-module.yml` deliberately have no such parameter: they never
check anything out and read their sources from the build artifact instead. See "The test jobs never
check out" below.

Everything that lives in the winget-cli tree is rooted at this parameter: the solution, the wapproj
files, the PowerShell scripts invoked by `filePath`, the `workingDirectory` of those tasks, the
`.csproj` restore glob, and the `tools\` tracing profiles.

Paths that belong to the agent or to the build output are deliberately **not** rooted at it, because
they do not move with the subtree:

- `$(Build.ArtifactStagingDirectory)`, `$(Pipeline.Workspace)`
- `$(Agent.TempDirectory)`, `$(VCPKG_INSTALLATION_ROOT)`
- `$(buildOutDir)`, `$(buildOutDirAnyCpu)`, `$(artifactsDir)`, `$(packageLayoutDir)`

`$(VCPKG_INSTALLATION_ROOT)` is the exception worth calling out: it is correctly *not* rooted, but a
consumer may not use the installation it points at. See `useAgentVcpkg` below.

Nested template references need no help. Azure Pipelines resolves a `- template:` reference inside a
template file relative to *that file*, so the job templates find their siblings at any depth. Only
the entry point has to know where the subtree lives.

## `WINGET_SOURCE_ROOT`

Not everything that needs the source root can be handed a task input. Some winget-cli content
resolves the sources at *runtime*, from inside a process the pipeline merely launches, and that
content used to read `BUILD_SOURCESDIRECTORY` — which is the root of the repository being built, and
so points outside the subtree.

All three job templates therefore export their source root as a job-scope variable named
`WINGET_SOURCE_ROOT`, which reaches every step as an environment variable. Two things read it:

- `src\AppInstallerCLIE2ETests\TestData\localsource.json`, whose `%WINGET_SOURCE_ROOT%` tokens locate
  the test installers and manifests that `LocalhostWebServer` bakes into `TestLocalIndex`.
- `Microsoft.Management.Configuration.UnitTests`, which uses it to find
  `src\PowerShell\ExternalModules`.

Nothing needs to be passed for this; in `jobs-build.yml` it follows `sourceRoot`, and in the two test
jobs it follows the artifact copy. It is listed here because it is the one part of the contract that
is neither a parameter nor a path in this directory.

## What a consuming pipeline must provide

**Pipeline variables.** None. `jobs-build.yml` sets what it needs at job scope — `solution`, so it
can be rooted, and `EnableDetectorVcpkg`, so Component Governance detects the vcpkg dependencies.
Both were previously pipeline-level variables in `azure-pipelines.yml`; a consumer that omitted
`EnableDetectorVcpkg` would have gotten a passing scan that silently skipped the native
dependencies, so the template owns it now.

**A Windows agent** with Visual Studio 2022 and vcpkg, exposing `VCPKG_INSTALLATION_ROOT`. The
Microsoft-hosted `windows-2025` image qualifies. **The `Test` and `BuildPowerShellModule` jobs need
more than the `Build` job does — see "Agent requirements" below.**

**Outbound network access.** The test jobs reach several public endpoints, which is worth checking
before wiring this up on a restricted network:

- PSGallery, for `Microsoft.WinGet.Client` and `platyPS`
- nuget.org, for the `VisualStudioTestPlatformInstaller` feed selector
- the `winget` source, for `Microsoft.Sysinternals.PsTools`

## Agent requirements

`Build` compiles and packages; it is happy on any Windows agent that meets the above.

`Test` and `BuildPowerShellModule` additionally **deploy MSIX packages and register them for the
current user**, which is a much stronger requirement than it looks. winget's packaged sources are
MSIX packages carrying the index as an
[app extension](https://learn.microsoft.com/windows/apps/desktop/modernize/desktop-to-uwp-extensions);
adding a source deploys the package, and opening the source enumerates the extension catalog. That
catalog is **per user**, so both halves need a real, interactive user session on the agent.

An agent whose service runs without one fails in a way that is easy to misread, because deployment
reports success:

```
Starting AddPackage operation #0 ... failed with error 0x80073D19
   80073D19 An error occurred because a user was logged off.
Source add/update failed, waiting 7584 milliseconds and retrying: TestSource
Starting AddPackage operation #1 ... Successfully completed #1
Did not find extension: PFN = WingetE2E.Tests_8wekyb3d8bbwe, ID = IndexDB
```

The package stages, never registers for a user, and the extension catalog comes back **empty** — no
`Examining extension` lines at all, not merely a non-matching one. Every test using a local source
then fails with `CatalogConnectException` or `APPINSTALLER_CLI_ERROR_SOURCE_DATA_MISSING`
(`0x8A15000F`). Machine-wide provisioning (`-AllUsers`) is unaffected, which can make the agent look
healthy right up until a source is added.

Microsoft-hosted images run the agent interactively and are fine. Many self-hosted and scale-set
pools run it as a service and are not. Each job template therefore takes a `pool` parameter, so a
consumer can send these two jobs somewhere suitable while building elsewhere:

```yaml
- template: templates/jobs-test.yml
  parameters:
    pool:
      vmImage: windows-2025
```

The parameter accepts whatever the `pool` key accepts — a `vmImage`, a `name`, `demands`. Left empty
(the default) the job inherits the pipeline's pool, so a pipeline with a single suitable pool passes
nothing and is unaffected.

Sending these jobs to a pool a consumer's sources must not be cloned onto costs nothing, because
they are never cloned onto any pool; see the next section.

## The test jobs never check out

`Test` and `BuildPowerShellModule` both declare `checkout: none` and read every source file they
need from the build artifact.

Two reasons. The jobs read scripts and test data that have to match the binaries they are testing,
and a checkout is not guaranteed to match: it resolves to whatever the consuming repository's
default branch or commit gives that job, which for a subtree consumer can drift from the sources the
artifact was built from. Taking the files from the artifact removes the question. It also means the
consuming repository is **never cloned onto these agents**, which matters when its sources are not
public and the jobs have to run on a hosted pool for the reasons above.

What makes it work is that these two jobs only ever *read* from the source tree — they compile
nothing. `jobs-build.yml` has a `Copy sources needed by the test jobs` step that stages exactly what
they read into `Source/` inside the artifact, **preserving the directory layout**, and the jobs point
their source root at it. Layout preservation is not incidental: several of these scripts navigate
relative to their own location, so a flattened copy would break them.

- `Microsoft.WinGet.Configuration.Tests.ps1` reaches test data through
  `$PSScriptRoot\..\..\AppInstallerCLIE2ETests\TestData\Configuration`
- `Microsoft.Management.Configuration.UnitTests` expects `<source root>\src\PowerShell\ExternalModules`

If a step in these two jobs ever comes to need another file from the tree, add it to that step's
`Contents` list; the failure mode is a missing-file error naming the path, under `…\Build.*\Source\`.

Steps in these jobs use a `$(wingetSourceRoot)` variable rather than `${{ parameters.sourceRoot }}`,
because the artifact path is only known at run time. That variable is set by the template and is not
a consumer's to choose, which is why neither job takes a `sourceRoot`.

## `preSteps`

Each job template takes a `preSteps` step list, injected ahead of every winget-cli step. It exists
because a job template cannot know what a consuming repository needs done to the agent first.

## `additionalMSBuildArgs`

`jobs-build.yml` appends this string to both of its build tasks.

`src\Directory.Build.props` imports any `Directory.Build.props` found above winget-cli, by design,
so a consumer's settings reach every project under `src\`. If those settings turn on the release
switches this repository defines — `WingetDisableTestHooks`, `UseProdCLSIDs`, `UseProdWingetServer`,
`WingetEnableReleaseBuild` — the result is a *product* build, not the build this pipeline's tests
expect. `WingetDisableTestHooks` compiles out members that the test projects use, so the solution
does not even build.

```yaml
- template: subtree/templates/jobs-build.yml
  parameters:
    additionalMSBuildArgs: >-
      /p:WingetDisableTestHooks=false
      /p:UseProdCLSIDs=false
```

These must be command line properties. A consumer that assigns the property in a `.props` file
assigns it unconditionally, and only a global property — which is what `/p:` creates — takes
precedence over that.

## `useAgentVcpkg`

Defaults to `true`, matching the Microsoft-hosted images, where `VCPKG_INSTALLATION_ROOT` points at
a real vcpkg installation that this pipeline integrates and builds the native dependencies with.

Set it to `false` when the consuming repository supplies vcpkg some other way — for example through
an MSBuild SDK that restores the ports itself. `VCPKG_INSTALLATION_ROOT` may well still be defined in
that case, since the agent image sets it, but nothing builds there. Two steps then become
meaningless and are dropped together:

- *Enable Vcpkg Install*, which integrates an installation the build does not use.
- *Copy vcpkg logs*, which reads `$(VCPKG_INSTALLATION_ROOT)\buildtrees` — a directory that will not
  exist, failing the job after a successful build.

Dropping the log collection loses no real diagnostics: `Build Solution` writes an MSBuild binary log
to `$(artifactsDir)\msbuild.binlog`, which is published with the rest of the build artifacts and
records the vcpkg invocation and its output.

## `releaseTagJob`

`jobs-build.yml` stamps a build version obtained from a separate job. That job is named by the
`releaseTagJob` parameter, which defaults to `GetReleaseTag`:

```yaml
- job: 'GetReleaseTag'
  steps:
  - task: PowerShell@2
    name: 'GetTag'          # the job must expose a 'tag' output under this step name
    ...
```

The name is load-bearing in two places — `dependsOn`, and the `BuildVer` counter that reads
`dependencies.<job>.outputs['GetTag.tag']`.

Pass an empty string when the consuming pipeline has no such job:

```yaml
- template: external/pkg/templates/jobs-build.yml
  parameters:
    releaseTagJob: ''
```

That drops the `dependsOn`, the `BuildVer` variable, and the *Update Binary Version* step together,
leaving the binaries with their checked-in version. For a validation-only pipeline that is usually
what you want, since nothing is being released.

## Job graph

`Build` publishes one artifact per matrix leg, named `Build.x86release` and `Build.x64release`.
`Test` and `BuildPowerShellModule` both depend on `Build` and consume those artifacts by name, so the
three jobs travel together.

```
[releaseTagJob] ──> Build ──┬──> Test
                            └──> BuildPowerShellModule
```

## Worked example

A validation pipeline in a repository that carries winget-cli at `subtree`:

```yaml
pool:
  vmImage: 'windows-2025'

jobs:
- template: subtree/templates/jobs-build.yml
  parameters:
    sourceRoot: $(Build.SourcesDirectory)\subtree
    releaseTagJob: ''

- template: subtree/templates/jobs-test.yml

- template: subtree/templates/jobs-powershell-module.yml
```

The `- template:` paths are relative to the consuming repository's root, since that is where the
entry pipeline lives. Only the build job takes a `sourceRoot`; the test jobs read their sources from
the artifact it publishes. The value is spelled out rather than held in a variable, because a
variable referenced from a template's `variables:` block relies on nested macro expansion that is
easy to get subtly wrong.

To stamp a version instead, add a job that produces the tag and name it in `releaseTagJob`;
`azure-pipelines.yml` in this repository is the worked example of that arrangement.

A consumer that has to build on its own pool but can only run the test jobs on a hosted agent sends
just those two elsewhere; nothing else changes, because they are already checkout-free:

```yaml
pool:
  name: MyBuildPool

jobs:
- template: subtree/templates/jobs-build.yml
  parameters:
    sourceRoot: $(Build.SourcesDirectory)\subtree
    releaseTagJob: ''

- template: subtree/templates/jobs-test.yml
  parameters:
    pool:
      vmImage: windows-2025

- template: subtree/templates/jobs-powershell-module.yml
  parameters:
    pool:
      vmImage: windows-2025
```

Only the build job needs a checkout, so only it takes `sourceRoot`.

## Editing these templates

The failure mode to watch for is a path that works at the repository root and only breaks in the
subtree, which this repository's own pipeline will not catch. When adding a task:

- Root anything inside the winget-cli tree at `${{ parameters.sourceRoot }}`, except in
  `jobs-test.yml` and `jobs-powershell-module.yml`, which use `$(wingetSourceRoot)` because their
  source comes from the build artifact. Adding a source file those two jobs read also means adding
  it to the `Copy sources needed by the test jobs` step in `jobs-build.yml`.
- Never leave a `filePath`, `solution`, `restoreSolution`, `projects`, or `workingDirectory` value
  as a repo-relative path such as `src\...`. Those resolve against the *consuming* repository.
- Prefer `${{ parameters.sourceRoot }}` over `$(Build.SourcesDirectory)` for source files. The latter
  is the consuming repository's root, which is only the same thing at depth zero.
- Watch globs. `**/*.csproj` looks harmless but would restore the entire consuming repository.
- Keep MSBuild work inside `src\`, which is where winget-cli's `Directory.Build.props` and
  `Directory.Packages.props` sit. See below.
