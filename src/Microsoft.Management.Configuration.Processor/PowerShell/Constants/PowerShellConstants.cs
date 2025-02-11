// -----------------------------------------------------------------------------
// <copyright file="PowerShellConstants.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.PowerShell.Constants
{
    /// <summary>
    /// Constants related to PowerShell.
    /// </summary>
    internal static class PowerShellConstants
    {
#pragma warning disable SA1600 // ElementsMustBeDocumented
        public const string Core = "Core";

        internal static class Variables
        {
            public const string PSEdition = "PSEdition";
            public const string Error = "Error";
            public const string PSModulePath = "env:PSModulePath";
        }

        internal static class Modules
        {
            public const string PSDesiredStateConfiguration = "PSDesiredStateConfiguration";
            public const string PSDesiredStateConfigurationMinVersion = "2.0.7";
            public const string PowerShellGet = "PowerShellGet";
            public const string PowerShellGetMinVersion = "2.2.5";
            public const string PSDesiredStateConfigurationMaxVersion = "2.*";
        }

        internal static class Commands
        {
            public const string FindDscResource = "Find-DscResource";
            public const string GetAuthenticodeSignature = "Get-AuthenticodeSignature";
            public const string GetChildItem = "Get-ChildItem";
            public const string GetCommand = "Get-Command";
            public const string GetDscResource = "Get-DscResource";
            public const string GetInstalledModule = "Get-InstalledModule";
            public const string GetModule = "Get-Module";
            public const string ImportModule = "Import-Module";
            public const string InstallModule = "Install-Module";
            public const string InvokeDscResource = "Invoke-DscResource";
            public const string SaveModule = "Save-Module";
            public const string FindModule = "Find-Module";
            public const string ImportCliXml = "Import-CliXml";
        }

        internal static class Parameters
        {
            public const string AllowPrerelease = "AllowPrerelease";
            public const string Force = "Force";
            public const string FullyQualifiedName = "FullyQualifiedName";
            public const string Guid = "GUID";
            public const string InputObject = "InputObject";
            public const string ListAvailable = "ListAvailable";
            public const string MaximumVersion = "MaximumVersion";
            public const string Method = "Method";
            public const string MinimumVersion = "MinimumVersion";
            public const string Module = "Module";
            public const string ModuleName = "ModuleName";
            public const string ModuleVersion = "ModuleVersion";
            public const string Name = "Name";
            public const string Path = "Path";
            public const string Property = "Property";
            public const string Recurse = "Recurse";
            public const string Repository = "Repository";
            public const string RequiredVersion = "RequiredVersion";
            public const string Scope = "Scope";
        }

        internal static class DscMethods
        {
            public const string Get = "Get";
            public const string Set = "Set";
            public const string Test = "Test";
        }
#pragma warning restore SA1600 // ElementsMustBeDocumented
    }
}
