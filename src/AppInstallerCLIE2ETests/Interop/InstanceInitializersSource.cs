// -----------------------------------------------------------------------------
// <copyright file="InstanceInitializersSource.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests.Interop
{
    using Microsoft.Management.Deployment.Projection;

    /// <summary>
    /// Source class for running tests in-process and out-of-process.
    /// </summary>
    public class InstanceInitializersSource
    {
        /// <summary>
        /// List of in-process instance initializers passed as argument to the test class constructor.
        /// </summary>
        public static readonly IInstanceInitializer[] InProcess =
        {
            new ActivationFactoryInstanceInitializer(),
        };

        /// <summary>
        /// List of out-of-process instance initializers passed as argument to the test class constructor.
        /// </summary>
        public static readonly IInstanceInitializer[] OutOfProcess =
        {
            new LocalServerInstanceInitializer()
            {
                AllowLowerTrustRegistration = true,
                UseDevClsids = true,
            },
        };
    }
}
