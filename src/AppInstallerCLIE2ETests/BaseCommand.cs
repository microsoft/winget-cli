// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using NUnit.Framework;

    public class BaseCommand
    {
        [OneTimeSetUp]
        public void Setup()
        {
            TestCommon.RunAICLICommand("source reset", "--force");
            TestCommon.RunAICLICommand("source remove", Constants.DefaultSourceName);
            TestCommon.RunAICLICommand("source add", $"{Constants.TestSourceName} {Constants.TestSourceUrl}");
        }

        [OneTimeTearDown]
        public void Teardown()
        {
            TestCommon.RunAICLICommand("source reset", "--force");
        }
    }
}
