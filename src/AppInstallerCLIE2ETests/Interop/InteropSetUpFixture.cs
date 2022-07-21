// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests.Interop
{
    using NUnit.Framework;
    using System;

    [SetUpFixture]
    public class InteropSetUpFixture
    {
        [OneTimeSetUp]
        public void Setup()
        {
            TestCommon.SetupTestSource();
        }

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
