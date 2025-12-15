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
        public enum ActivationType
        {
            /// <summary>
            /// Use the WinRT type name for activation via C++/WinRT object construction.
            /// </summary>
            ClassName,

            /// <summary>
            /// Use the CLSID for activation via C++/WinRT `create_instance`.
            /// </summary>
            CoCreateInstance,
        }

        /// <summary>
        /// Control when the module will allow signal that it can be unloaded if all objects are released.
        /// This does not affect the loader by taking additional references to the module.
        /// </summary>
        public enum UnloadBehavior
        {
            /// <summary>
            /// Allows the unload check function to proceed with object count checks and unload when possible.
            /// </summary>
            Allow,

            /// <summary>
            /// Prevents the unload check until just before COM is uninitialized.
            /// </summary>
            AtUninitialize,

            /// <summary>
            /// Prevents the unload check at all times.
            /// </summary>
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
                this.InprocTestbedPath = Path.Join(Path.GetDirectoryName(assemblyLocation), "..\\ComInprocTestbed\\ComInprocTestbed.exe");
            }

            if (TestSetup.Parameters.InprocTestbedUseTestPackage)
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

        /// <summary>
        /// Tests using the CLSID with CoCreateInstance.
        /// </summary>
        /// <param name="leakCOM">Control whether COM should be uninitialized at the end of the process.</param>
        /// <param name="unloadBehavior">Set the unload behavior for the test.</param>
        [Test]
        [TestCase(false, UnloadBehavior.AtUninitialize)]
        [TestCase(false, UnloadBehavior.Never)]
        [TestCase(true, UnloadBehavior.Allow)]
        [TestCase(true, UnloadBehavior.Never)]
        public void CLSID_Tests(bool leakCOM, UnloadBehavior unloadBehavior)
        {
            this.RunInprocTestbed(new TestbedParameters()
            {
                ActivationType = ActivationType.CoCreateInstance,
                LeakCOM = leakCOM,
                UnloadBehavior = unloadBehavior,
                Iterations = 10,
            });
        }

        /// <summary>
        /// Tests using C++/WinRT object activation through the type name.
        /// </summary>
        /// <param name="freeCachedFactories">Control whether the C++/WinRT factory cache will be cleared between iterations.</param>
        /// <param name="leakCOM">Control whether COM should be uninitialized at the end of the process.</param>
        /// <param name="unloadBehavior">Set the unload behavior for the test.</param>
        [Test]
        [TestCase(false, false, UnloadBehavior.AtUninitialize)]
        [TestCase(false, false, UnloadBehavior.Never)]
        [TestCase(false, true, UnloadBehavior.Allow)]
        [TestCase(false, true, UnloadBehavior.Never)]
        [TestCase(true, false, UnloadBehavior.AtUninitialize)]
        [TestCase(true, false, UnloadBehavior.Never)]
        [TestCase(true, true, UnloadBehavior.Allow)]
        [TestCase(true, true, UnloadBehavior.Never)]
        public void TypeName_Tests(bool freeCachedFactories, bool leakCOM, UnloadBehavior unloadBehavior)
        {
            this.RunInprocTestbed(new TestbedParameters()
            {
                ActivationType = ActivationType.ClassName,
                ClearFactories = freeCachedFactories,
                LeakCOM = leakCOM,
                UnloadBehavior = unloadBehavior,
                Iterations = 10,
            });
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
                builtParameters += $"-unload {parameters.UnloadBehavior} ";
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
