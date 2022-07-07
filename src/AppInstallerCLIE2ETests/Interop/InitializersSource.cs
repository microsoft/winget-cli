namespace AppInstallerCLIE2ETests.Interop
{
    using Microsoft.Management.Deployment.Projection;

    /// <summary>
    /// Source class for running tests in-process and out-of-process
    /// </summary>
    public class InitializersSource
    {
        /// <summary>
        /// List of in-process initializers passed as argument to the test class constructor
        /// </summary>
        public static readonly IInstanceInitializer[] InProcess =
        {
            new ActivationFactoryInitializer()
        };

        /// <summary>
        /// List of out-of-process initializers passed as argument to the test class constructor
        /// </summary>
        public static readonly IInstanceInitializer[] OutOfProcess =
        {
            new LocalServerInitializer()
            {
                AllowLowerTrustRegistration = true,
                UseDevClsids = true
            }
        };
    }
}
