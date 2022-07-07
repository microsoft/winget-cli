namespace AppInstallerCLIE2ETests.Interop
{
    using Microsoft.Management.Deployment.Projection;

    /// <summary>
    /// Source class for running tests in-process and out-of-process
    /// </summary>
    public class InitializersSource
    {
        // List of in-process initializers passed as argument to the test class constructor
        public static IInstanceInitializer[] InProcess =
        {
            new ActivationFactoryInitializer()
        };

        // List of out-of-process initializers passed as argument to the test class constructor
        public static IInstanceInitializer[] OutOfProcess =
        {
            new LocalServerInitializer(useDevClsids: true)
            {
                AllowLowerTrustRegistration = true
            }
        };
    }
}
