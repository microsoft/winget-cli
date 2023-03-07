// -----------------------------------------------------------------------------
// <copyright file="Constants.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Common
{
    using Microsoft.Management.Deployment;

    /// <summary>
    /// This class contains all of the configurable constants for this project.
    /// </summary>
    internal static class Constants
    {
        /// <summary>
        /// If a command allows the specification of the maximum number of results to return, this is the lower bound for that value.
        /// </summary>
        public const uint CountLowerBound = 1;

        /// <summary>
        /// If a command allows the specification of the maximum number of results to return, this is the upper bound for that value.
        /// </summary>
        public const uint CountUpperBound = 1000;

        /// <summary>
        /// This parameter set indicates that a package was provided via a parameter or the pipeline and it can be acted on directly.
        /// </summary>
        public const string GivenSet = "GivenSet";

        /// <summary>
        /// This parameter set indicates that a package was not provided via a parameter or the pipeline and it
        /// needs to be found by searching a package source.
        /// </summary>
        public const string FoundSet = "FoundSet";

        /// <summary>
        /// Parameter set for an specific version parameter.
        /// </summary>
        public const string IntegrityVersionSet = "IntegrityVersionSet";

        /// <summary>
        /// Parameter set for an latest version with optional prerelease version.
        /// </summary>
        public const string IntegrityLatestSet = "IntegrityLatestSet";

        /// <summary>
        /// WinGet package family name.
        /// </summary>
#if USE_PROD_CLSIDS
        public const string WingetPackageFamilyName = "Microsoft.DesktopAppInstaller_8wekyb3d8bbwe";
#else
        public const string WingetPackageFamilyName = "WinGetDevCLI_8wekyb3d8bbwe";
#endif

        /// <summary>
        /// Winget executable name.
        /// </summary>
#if USE_PROD_CLSIDS
        public const string WinGetExe = "winget.exe";
#else
        public const string WinGetExe = "wingetdev.exe";
#endif

        /// <summary>
        /// Name of PATH environment variable.
        /// </summary>
        public const string PathEnvVar = "PATH";

        /// <summary>
        /// Nouns used for different cmdlets. Changing this will alter the names of the related commands.
        /// </summary>
        public static class WinGetNouns
        {
            /// <summary>
            /// WinGet.
            /// </summary>
            public const string WinGetPackageManager = "WinGetPackageManager";

            /// <summary>
            /// The noun analogue of the <see cref="CatalogPackage" /> class.
            /// </summary>
            public const string Package = "WinGetPackage";

            /// <summary>
            /// The noun analogue of the <see cref="PackageCatalogReference" /> class.
            /// </summary>
            public const string Source = "WinGetSource";

            /// <summary>
            /// The noun for any user settings cmdlet.
            /// </summary>
            public const string UserSettings = "WinGetUserSettings";

            /// <summary>
            /// The noun for winget version.
            /// </summary>
            public const string Version = "WinGetVersion";
        }
    }
}
