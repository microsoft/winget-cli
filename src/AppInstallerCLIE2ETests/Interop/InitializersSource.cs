namespace AppInstallerCLIE2ETests.Interop
{
    using Microsoft.Management.Deployment.Projection;

    internal class InitializersSource
    {
        // Use <buildDir>\winget.exe to run InProcess tests
        public static IInstanceInitializer[] InProcess =
        {
            new ActivationFactoryInitializer()
        };

        // Use WinGetDev.exe to run OutOfProcess tests
        public static IInstanceInitializer[] OutOfProcess =
        {
            new LocalServerInitializer(useDevClsids: true)
            {
                AllowLowerTrustRegistration = true
            }
        };
    }
}
