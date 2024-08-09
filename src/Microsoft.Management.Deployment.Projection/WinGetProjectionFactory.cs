// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace Microsoft.Management.Deployment.Projection
{
    /// <summary>
    /// Factory class to created CsWinRT projected class instances for in-process or out-of-process objects.
    /// </summary>
    public class WinGetProjectionFactory
    {
        public WinGetProjectionFactory(IInstanceInitializer instanceInitializer)
        {
            InstanceInitializer = instanceInitializer;
        }

        private IInstanceInitializer InstanceInitializer { get; set; }

        public ClsidContext Context => InstanceInitializer.Context;

        public PackageManager CreatePackageManager() => InstanceInitializer.CreateInstance<PackageManager>();

        public FindPackagesOptions CreateFindPackagesOptions() => InstanceInitializer.CreateInstance<FindPackagesOptions>();

        public CreateCompositePackageCatalogOptions CreateCreateCompositePackageCatalogOptions() => InstanceInitializer.CreateInstance<CreateCompositePackageCatalogOptions>();

        public InstallOptions CreateInstallOptions() => InstanceInitializer.CreateInstance<InstallOptions>();

        public UninstallOptions CreateUninstallOptions() => InstanceInitializer.CreateInstance<UninstallOptions>();

        public DownloadOptions CreateDownloadOptions() => InstanceInitializer.CreateInstance<DownloadOptions>();

        public PackageMatchFilter CreatePackageMatchFilter() => InstanceInitializer.CreateInstance<PackageMatchFilter>();

        public AuthenticationArguments CreateAuthenticationArguments() => InstanceInitializer.CreateInstance<AuthenticationArguments>();

        public PackageManagerSettings CreatePackageManagerSettings() => InstanceInitializer.CreateInstance<PackageManagerSettings>();
    }
}
