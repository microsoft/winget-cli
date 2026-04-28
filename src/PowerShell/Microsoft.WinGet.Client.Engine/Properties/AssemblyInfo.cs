// -----------------------------------------------------------------------------
// <copyright file="AssemblyInfo.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

using System.Runtime.CompilerServices;
#if NET
using System.Runtime.Versioning;
#endif

[assembly: InternalsVisibleTo("Microsoft.WinGet.UnitTests")]

#if NET

// Forcibly set the target and supported platforms due to the internal build setup.
// Keep in sync with project versions.
[assembly: TargetPlatform("Windows10.0.26100.0")]
[assembly: SupportedOSPlatform("Windows10.0.18362.0")]

#endif
