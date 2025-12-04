// -----------------------------------------------------------------------------
// <copyright file="InprocTestbedTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
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
        /// The activation type to use when creating objects.
        /// </summary>
        private enum ActivationType
        {
            ClassName,
            CoCreateInstance,
        }

        /// <summary>
        /// Control when the module will allow signal that it can be unloaded if all objects are released.
        /// </summary>
        private enum UnloadBehavior
        {
            Allow,
            AtExit,
            Never,
        }

        /// <summary>
        /// Gets or sets the path to the inproc testbed executable.
        /// </summary>
        private string InprocTestbedPath { get; set; }

        /// <summary>
        /// Gets or sets the string that contains the package identity to use for the tests.
        /// </summary>
        private string TargetPackageInformation { get; set; }

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

            // If we are using the test source, target a package in it
            if (!TestSetup.Parameters.SkipTestSource)
            {
                this.TargetPackageInformation = $"-pkg {Constants.ExeInstallerPackageId} -src {Constants.TestSourceName} -url {Constants.TestSourceUrl}";
            }
        }

        /// <summary>
        /// Executes the testbed as simply as possible to ensure integrations.
        /// </summary>
        [Test]
        public void DefaultTest()
        {
            this.RunInprocTestbed(new TestbedParameters());
        }

        private void RunInprocTestbed(TestbedParameters parameters, int timeout = 300000)
        {
            string builtParameters = string.Empty;

            if (parameters.ActivationType != null)
            {
                builtParameters += $"-activation {parameters.ActivationType} ";
            }

            if (!parameters.ClearFactories)
            {
                builtParameters += "-keep-factories ";
            }

            if (parameters.LeakCOM)
            {
                builtParameters += "-leak-com ";
            }

            if (parameters.UnloadBehavior != null)
            {
                builtParameters += $"-unload {parameters.ActivationType} ";
            }

            if (parameters.Test != null)
            {
                builtParameters += $"-test {parameters.Test} ";
            }

            if (parameters.Iterations != null)
            {
                builtParameters += $"-itr {parameters.Iterations} ";
            }

            var result = TestCommon.RunProcess(this.InprocTestbedPath, this.TargetPackageInformation, builtParameters, null, timeout, true);
            Assert.AreEqual(0, result.ExitCode);
        }

        /// <summary>
        /// The parameters to provide for running tests.
        /// </summary>
        private class TestbedParameters
        {
            internal ActivationType? ActivationType { get; init; } = null;

            internal bool ClearFactories { get; init; } = true;

            internal bool LeakCOM { get; init; } = false;

            internal UnloadBehavior? UnloadBehavior { get; init; } = null;

            internal string Test { get; init; } = "unload_check";

            internal int? Iterations { get; init; } = null;
        }
    }
}
