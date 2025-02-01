// -----------------------------------------------------------------------------
// <copyright file="AssemblyInfo.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

using System.Runtime.CompilerServices;
using System.Runtime.Versioning;

// InternalsVisibleTo specifies that types that are ordinarily visible only within the current
// assembly are visible to a specified assembly. This is only for types and members with internal
// or private protected scope, NOT private. Add any test dll that requires access to internal members.
[assembly: InternalsVisibleTo("Microsoft.Management.Configuration.UnitTests")]

// Needed to allow us mock internal interfaces.
[assembly: InternalsVisibleTo("DynamicProxyGenAssembly2")]

#if WinGetCsWinRTEmbedded
// Allow our consuming assemblies access when built embedded.
[assembly: InternalsVisibleTo("Microsoft.WinGet.Configuration.Engine")]
[assembly: InternalsVisibleTo("ConfigurationRemotingServer")]
#endif

// Forcibly set the target and supported platforms due to the internal build setup.
// Keep in sync with project versions.
[assembly: TargetPlatform("Windows10.0.22000.0")]
[assembly: SupportedOSPlatform("Windows10.0.17763.0")]
