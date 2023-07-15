// -----------------------------------------------------------------------------
// <copyright file="InteropSetUpFixture.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests.Interop
{
    using System;
    using NUnit.Framework;

    /// <summary>
    /// Interop set up fixture.
    /// </summary>
    [SetUpFixture]
    public class InteropSetUpFixture
    {
        /// <summary>
        /// One time set up.
        /// </summary>
        [OneTimeSetUp]
        public void Setup()
        {
            TestCommon.SetupTestSource();

            // For the COM E2E tests, once the settings file is initialized, it persists throughout the entirety of the tests.
            // Any experimental features needed must be declared prior to running the COM E2E tests.
            WinGetSettingsHelper.ConfigureFeature("dependencies", true);
            WinGetSettingsHelper.ConfigureFeature("download", true);
        }

        /// <summary>
        /// Tear down.
        /// </summary>
        [OneTimeTearDown]
        public void TearDown()
        {
            try
            {
                GC.Collect();
                GC.WaitForPendingFinalizers();

                TestCommon.TearDownTestSource();
            }
            catch
            {
                // If the COM objects were not yet disposed and lock acquired
                // when connecting to test source was not released, then just
                // exit process since all tests have already executed.
            }
        }
    }
}
