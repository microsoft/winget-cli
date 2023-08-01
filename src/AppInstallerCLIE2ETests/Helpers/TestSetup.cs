// -----------------------------------------------------------------------------
// <copyright file="TestSetup.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests.Helpers
{
    using System;
    using System.IO;
    using NUnit.Framework;

    /// <summary>
    /// Singleton class with test parameters.
    /// </summary>
    internal class TestSetup
    {
        private static readonly Lazy<TestSetup> Lazy = new (() => new TestSetup());

        private string settingFilePath = null;

        private TestSetup()
        {
            if (TestContext.Parameters.Count == 0)
            {
                this.IsDefault = true;
            }

            // Read TestParameters and set runtime variables
            this.PackagedContext = this.InitializeBoolParam(Constants.PackagedContextParameter, true);
            this.VerboseLogging = this.InitializeBoolParam(Constants.VerboseLoggingParameter, true);
            this.LooseFileRegistration = this.InitializeBoolParam(Constants.LooseFileRegistrationParameter);
            this.InvokeCommandInDesktopPackage = this.InitializeBoolParam(Constants.InvokeCommandInDesktopPackageParameter);
            this.SkipTestSource = this.InitializeBoolParam(Constants.SkipTestSourceParameter, this.IsDefault);

            // For packaged context, default to AppExecutionAlias
            this.AICLIPath = this.InitializeStringParam(Constants.AICLIPathParameter, this.PackagedContext ? "WinGetDev.exe" : TestCommon.GetTestFile("winget.exe"));
            this.AICLIPackagePath = this.InitializeStringParam(Constants.AICLIPackagePathParameter, TestCommon.GetTestFile("AppInstallerCLIPackage.appxbundle"));

            if (this.LooseFileRegistration && this.InvokeCommandInDesktopPackage)
            {
                this.AICLIPath = Path.Combine(this.AICLIPackagePath, this.AICLIPath);
            }

            this.StaticFileRootPath = this.InitializeDirectoryParam(Constants.StaticFileRootPathParameter, Path.GetTempPath());

            this.PowerShellModuleManifestPath = this.InitializeFileParam(Constants.PowerShellModulePathParameter);
            this.LocalServerCertPath = this.InitializeFileParam(Constants.LocalServerCertPathParameter);
            this.PackageCertificatePath = this.InitializeFileParam(Constants.PackageCertificatePathParameter);
            this.ExeInstallerPath = this.InitializeFileParam(Constants.ExeInstallerPathParameter);
            this.MsiInstallerPath = this.InitializeFileParam(Constants.MsiInstallerPathParameter);
            this.MsixInstallerPath = this.InitializeFileParam(Constants.MsixInstallerPathParameter);
        }

        /// <summary>
        /// Gets the instance object.
        /// </summary>
        public static TestSetup Parameters
        {
            get
            {
                return Lazy.Value;
            }
        }

        /// <summary>
        /// Gets the cli path.
        /// </summary>
        public string AICLIPath { get; }

        /// <summary>
        /// Gets the package path.
        /// </summary>
        public string AICLIPackagePath { get; }

        /// <summary>
        /// Gets a value indicating whether the test runs in package context.
        /// </summary>
        public bool PackagedContext { get; }

        /// <summary>
        /// Gets a value indicating whether the test uses verbose logging.
        /// </summary>
        public bool VerboseLogging { get; }

        /// <summary>
        /// Gets a value indicating whether to use loose file registration.
        /// </summary>
        public bool LooseFileRegistration { get; }

        /// <summary>
        /// Gets a value indicating whether to invoke command in desktop package.
        /// </summary>
        public bool InvokeCommandInDesktopPackage { get; }

        /// <summary>
        /// Gets the static file root path.
        /// </summary>
        public string StaticFileRootPath { get; }

        /// <summary>
        /// Gets the local server cert path.
        /// </summary>
        public string LocalServerCertPath { get; }

        /// <summary>
        /// Gets the exe installer path.
        /// </summary>
        public string ExeInstallerPath { get; }

        /// <summary>
        /// Gets the msi installer path.
        /// </summary>
        public string MsiInstallerPath { get; }

        /// <summary>
        /// Gets the msix installer path.
        /// </summary>
        public string MsixInstallerPath { get; }

        /// <summary>
        /// Gets the zip installer path.
        /// </summary>
        public string ZipInstallerPath { get; }

        /// <summary>
        /// Gets the package cert path.
        /// </summary>
        public string PackageCertificatePath { get; }

        /// <summary>
        /// Gets the PowerShell module path.
        /// </summary>
        public string PowerShellModuleManifestPath { get; }

        /// <summary>
        /// Gets a value indicating whether to skip creating test source.
        /// </summary>
        public bool SkipTestSource { get; }

        /// <summary>
        /// Gets the settings json path.
        /// </summary>
        public string SettingsJsonFilePath
        {
            get
            {
                if (this.settingFilePath == null)
                {
                    this.settingFilePath = WinGetSettingsHelper.GetUserSettingsPath();
                }

                return this.settingFilePath;
            }
        }

        /// <summary>
        /// Gets a value indicating whether is the default parameters.
        /// </summary>
        public bool IsDefault { get; }

        private bool InitializeBoolParam(string paramName, bool defaultValue = false)
        {
            if (this.IsDefault || !TestContext.Parameters.Exists(paramName))
            {
                return defaultValue;
            }

            return TestContext.Parameters.Get(paramName).Equals("true", StringComparison.OrdinalIgnoreCase);
        }

        private string InitializeStringParam(string paramName, string defaultValue = null)
        {
            if (this.IsDefault || !TestContext.Parameters.Exists(paramName))
            {
                return defaultValue;
            }

            return TestContext.Parameters.Get(paramName);
        }

        private string InitializeFileParam(string paramName, string defaultValue = null)
        {
            if (!TestContext.Parameters.Exists(paramName))
            {
                return defaultValue;
            }

            var value = TestContext.Parameters.Get(paramName);

            if (!File.Exists(value))
            {
                throw new FileNotFoundException($"{paramName}: {value}");
            }

            return value;
        }

        private string InitializeDirectoryParam(string paramName, string defaultValue = null)
        {
            if (!TestContext.Parameters.Exists(paramName))
            {
                return defaultValue;
            }

            var value = TestContext.Parameters.Get(paramName);

            if (!Directory.Exists(value))
            {
                throw new DirectoryNotFoundException($"{paramName}: {value}");
            }

            return value;
        }
    }
}
