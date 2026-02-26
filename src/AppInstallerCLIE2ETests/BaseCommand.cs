// -----------------------------------------------------------------------------
// <copyright file="BaseCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using AppInstallerCLIE2ETests.Helpers;
    using NUnit.Framework;

    /// <summary>
    /// Base command.
    /// </summary>
    public class BaseCommand
    {
        /// <summary>
        /// Set up.
        /// </summary>
        [OneTimeSetUp]
        public void BaseSetup()
        {
            this.ResetTestSource();
        }

        /// <summary>
        /// Tear down.
        /// </summary>
        [OneTimeTearDown]
        public void BaseTeardown()
        {
            TestCommon.TearDownTestSource();
        }

        /// <summary>
        /// Reset test source.
        /// </summary>
        /// <param name="useGroupPolicyForTestSource">Use group policy from test source.</param>
        public void ResetTestSource(bool useGroupPolicyForTestSource = false)
        {
            // TODO: If/when cert pinning is implemented on the packaged index source, useGroupPolicyForTestSource should be set to default true
            //       to enable testing it by default.  Until then, leaving this here...
            TestCommon.SetupTestSource(useGroupPolicyForTestSource);
        }
    }
}
