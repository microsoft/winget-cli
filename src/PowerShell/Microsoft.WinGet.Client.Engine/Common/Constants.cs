// -----------------------------------------------------------------------------
// <copyright file="Constants.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Common
{
    /// <summary>
    /// This class contains all of the configurable constants for this project.
    /// </summary>
    internal static class Constants
    {
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
    }
}
