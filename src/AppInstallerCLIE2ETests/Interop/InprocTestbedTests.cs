// -----------------------------------------------------------------------------
// <copyright file="InprocTestbedTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using System;
    using System.IO;
    using System.Reflection;
    using AppInstallerCLIE2ETests.Helpers;
    using NUnit.Framework;

    /// <summary>
    /// Tests that run the inproc testbed targeting COM lifetime.
    /// </summary>
    public class InprocTestbedTests
    {
        /// <summary>
        /// Gets or sets the path to the inproc testbed executable.
        /// </summary>
        private string InprocTestbedPath { get; set; }

        /// <summary>
        /// Setup done once before all the tests here.
        /// </summary>
        [OneTimeSetUp]
        public void OneTimeSetup()
        {
            this.InprocTestbedPath = TestSetup.Parameters.InprocTestbedPath;

            if (string.IsNullOrWhiteSpace(this.InprocTestbedPath))
            {
                string assemblyLocation = Assembly.GetExecutingAssembly().Location;
                this.InprocTestbedPath = Path.Combine(Path.GetDirectoryName(assemblyLocation), "..\\ComInprocTestbed\\ComInprocTestbed.exe");
            }
        }

        /// <summary>
        /// Executes the testbed as simply as possible to ensure integrations.
        /// </summary>
        [Test]
        public void DefaultTest()
        {
            RunInprocTestbed(new TestbedParameters());
        }

        private enum ActivationType
        {
            ClassName,
            CoCreateInstance
        }

        private enum UnloadBehavior
        {
            Allow,
            AtExit,
            Never,
        };

        private class TestbedParameters
        {
            ActivationType? ActivationType { get; init; } = null;
            bool? ClearFactories { get; init; } = null;
            bool? LeakCOM { get; init; } = null;
            UnloadBehavior? UnloadBehavior { get; init; } = null;
            string Test { get; init; } = "unload_check";
            int? Iterations { get; init; } = null;
        }

        private void RunInprocTestbed(TestbedParameters parameters)
        {

        }
    }
}
