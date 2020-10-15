// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using NUnit.Framework;
    using System.Threading;

    public class BaseCommand
    {
        [OneTimeSetUp]
        public void Setup()
        {
            ResetTestSource();
        }

        [OneTimeTearDown]
        public void Teardown()
        {
            TestCommon.RunAICLICommand("source reset", "--force");
        }

        public void ResetTestSource()
        {
            TestCommon.RunAICLICommand("source reset", "--force");
            TestCommon.RunAICLICommand("source remove", Constants.DefaultSourceName);
            TestCommon.RunAICLICommand("source add", $"{Constants.TestSourceName} {Constants.TestSourceUrl}");
            Thread.Sleep(5000);
        }
    }
}
