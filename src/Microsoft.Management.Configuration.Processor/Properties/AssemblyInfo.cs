// -----------------------------------------------------------------------------
// <copyright file="AssemblyInfo.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

using System.Runtime.CompilerServices;

// InternalsVisibleTo specifies that types that are ordinarily visible only within the current
// assembly are visible to a specified assembly. This is only for types and members with internal
// or private protected scope, NOT private. Add any test dll that requires access to internal members.
[assembly: InternalsVisibleTo("Microsoft.Management.Configuration.UnitTests")]

// Needed to allow us mock internal interfaces.
[assembly: InternalsVisibleTo("DynamicProxyGenAssembly2")]
