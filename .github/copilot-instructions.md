# WinGet CLI Development Guide

## Project Overview

This is the Windows Package Manager (WinGet) CLI client - a native Windows application for discovering and installing packages. The codebase consists of:

- **C++/WinRT client** (`src/AppInstallerCLI*`) - The main CLI and core logic
- **COM API** (`src/Microsoft.Management.Deployment`) - Public Windows Runtime API for programmatic access
- **PowerShell modules** (`src/PowerShell`) - Microsoft.WinGet.Client and Microsoft.WinGet.Configuration cmdlets
- **Configuration system** - DSC-based system configuration using WinGet

## Building, Testing, and Running

### Initial Setup

1. Install Visual Studio 2022 with required workloads (see `.vsconfig`)
2. Install Windows SDK 10.0.26100: `winget install Microsoft.WindowsSDK.10.0.26100`
3. Configure system: `winget configure .config/configuration.winget` (from project root)
4. Run `vcpkg integrate install` from Developer Command Prompt

### Building

Open `src\AppInstallerCLI.sln` in Visual Studio and build the solution (Ctrl+Shift+B).

The solution uses:
- MSBuild for native C++ projects
- vcpkg for C++ dependencies
- NuGet for .NET dependencies

### Running/Debugging

1. Deploy solution: Build > Deploy Solution
2. Run from command line: `wingetdev`
3. For debugging:
   - Right-click `AppInstallerCLIPackage` > Properties > Debug tab
   - Set Debugger type to "Native Only" for both Application and Background task processes
   - Select "Do not launch, but debug my code when it starts"
   - Press F5 and run `wingetdev` in a separate terminal

Entry point: `src/AppInstallerCLI/main.cpp`

### Testing

#### C++ Unit Tests (Catch2)
Located in `AppInstallerCLITests` project. After building:

```powershell
# Run all tests
src\<ARCH>\<Debug|Release>\AppInstallerCLITests\AppInstallerCLITests.exe

# Run specific test
src\<ARCH>\<Debug|Release>\AppInstallerCLITests\AppInstallerCLITests.exe TestName

# Available options
AppInstallerCLITests.exe --help
```

#### .NET Tests
- `Microsoft.WinGet.UnitTests` - PowerShell module tests
- `Microsoft.Management.Configuration.UnitTests` - Configuration system tests
- `WinGetUtilInterop.UnitTests` - Interop layer tests

#### E2E Tests
`AppInstallerCLIE2ETests` project contains end-to-end integration tests.

## Architecture

### Core Components

**AppInstallerCLICore** - Core CLI logic organized around:
- **ExecutionContext**: State container that flows through workflows. Contains arguments, reporter, flags, and data (ExecutionContextData.h)
- **Workflows**: Composable functions that take ExecutionContext and perform operations (e.g., InstallFlow, UpdateFlow, SearchFlow)
- **Commands**: Parse arguments and orchestrate workflows
- **Reporter**: Handles all user output (ExecutionReporter.h)

**AppInstallerRepositoryCore** - Package source abstraction:
- Interfaces for different source types (REST, SQLite index, Microsoft Store, composite)
- Search, match, and correlation logic
- Package version selection and dependencies

**AppInstallerCommonCore** - Shared utilities:
- Manifest parsing (YAML/JSON)
- Settings and group policy
- Telemetry and logging
- HTTP client, downloader, archive handling

**Microsoft.Management.Deployment** - COM API surface:
- IDL definitions in `PackageManager.idl`
- WinRT projections for external consumption
- Used by PowerShell modules and third-party integrations

**AppInstallerCLIPackage** - Dev MSIX package definition:
- Models the release package definition as closely as possible.
- Contains localized string resources at src\AppInstallerCLIPackage\Shared\Strings\en-us\winget.resw

### Key Patterns

**Workflow Pattern**: Functions that operate on ExecutionContext:
```cpp
void WorkflowTask(Execution::Context& context)
{
    // Check if already terminated
    AICLI_RETURN_IF_TERMINATED(context);
    
    // Access data
    auto& data = context.Get<Data::Installer>();
    
    // Report to user
    context.Reporter.Info() << "Doing something";
    
    // Store data for next workflow
    context.Add<Data::SomeResult>(result);
    
    // Terminate on error
    if (failed)
    {
        AICLI_TERMINATE_CONTEXT(HRESULT);
    }
}
```

**Source Composition**: Multiple package sources can be composed:
- CompositeSource combines multiple sources with conflict resolution
- Installed source tracks locally installed packages
- Available sources provide packages to install

**Manifest Schema**: Package manifests use versioned YAML schemas:
- Schema definitions in `schemas/JSON/manifests/`
- Parsing in `AppInstallerCommonCore/Manifest/`
- Multi-file manifests: installer, locale, version, defaultLocale

## Naming Conventions

- **Namespace structure**: `AppInstaller::<Area>[::<Subarea>]`
  - `AppInstaller::CLI::Execution` - CLI execution context
  - `AppInstaller::CLI::Workflow` - Workflow functions
  - `AppInstaller::Repository` - Repository/source logic
  - `AppInstaller::Manifest` - Manifest types
  - `AppInstaller::Settings` - User/admin settings

- **Macros**: Prefixed with `AICLI_` for CLI, `WINGET_` for general
- **Data keys**: ExecutionContextData uses enum keys to type-safely store/retrieve data

## Windows-Specific Considerations

- Use Windows-style paths with backslashes (`\`)
- Leverage WinRT APIs via C++/WinRT projections
- COM threading models matter - client uses multi-threaded apartment (MTA)
- Package deployment uses Windows App SDK / MSIX infrastructure
- Requires Windows 10 1809+ (build 17763)

## Contributing

- Review `CONTRIBUTING.md` for workflow
- File/discuss issues before starting work
- Specs required for features (stored in `doc/specs/`)
- Follow existing code style (see `stylecop.json`)
- CI runs on Azure Pipelines (`azure-pipelines.yml`)

## Useful Commands

```powershell
# Get WinGet client info
wingetdev --info

# Run with experimental features
wingetdev features

# Check sources
wingetdev source list
```
