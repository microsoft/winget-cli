// -----------------------------------------------------------------------------
// <copyright file="ServerConnection.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace WinGetMCPServer
{
    using Microsoft.Management.Deployment;

    /// <summary>
    /// Maintains the connection to the server.
    /// </summary>
    internal static class ServerConnection
    {
        private static PackageManager? packageManager = null;

        public static PackageManager Instance
        {
            get
            {
                if (packageManager == null)
                {
                    packageManager = new PackageManager();
                }

                try
                {
                    _ = packageManager.Version;
                }
                catch
                {
                    packageManager = new PackageManager();
                }

                return packageManager;
            }
        }
    }
}
