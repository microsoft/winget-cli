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
    /// Singleton class with test paramters.
    /// </summary>
    internal class TestSetup
    {
        private static readonly Lazy<TestSetup> Lazy = new (() => new TestSetup());

        private TestSetup()
        {
            if (TestContext.Parameters.Count == 0)
            {
                this.IsDefault = true;
            }

            this.SettingsJsonFilePath = WinGetSettingsHelper.GetUserSettingsPath();

            // Read TestParameters and set runtime variables
            this.PackagedContext = this.Initialize(Constants.PackagedContextParameter, true);
            this.VerboseLogging = this.Initialize(Constants.VerboseLoggingParameter, true);
            this.LooseFileRegistration = this.Initialize(Constants.LooseFileRegistrationParameter, false);
            this.InvokeCommandInDesktopPackage = this.Initialize(Constants.InvokeCommandInDesktopPackageParameter, false);

            // For packaged context, default to AppExecutionAlias
            this.AICLIPath = this.Initialize(Constants.AICLIPathParameter, this.PackagedContext ? "WinGetDev.exe" : TestCommon.GetTestFile("winget.exe"));
            this.AICLIPackagePath = this.Initialize(Constants.AICLIPackagePathParameter, TestCommon.GetTestFile("AppInstallerCLIPackage.appxbundle"));

            if (this.LooseFileRegistration && this.InvokeCommandInDesktopPackage)
            {
                this.AICLIPath = Path.Combine(this.AICLIPackagePath, this.AICLIPath);
            }

            this.StaticFileRootPath = this.Initialize(Constants.StaticFileRootPathParameter, Path.GetTempPath());
            this.LocalServerCertPath = this.Initialize(Constants.LocalServerCertPathParameter, string.Empty);
            this.PackageCertificatePath = this.Initialize(Constants.PackageCertificatePathParameter, string.Empty);
            this.PowerShellModulePath = this.Initialize(Constants.PowerShellModulePathParameter, string.Empty);
            this.ExeInstallerPath = this.Initialize(Constants.ExeInstallerPathParameter, string.Empty);
            this.MsiInstallerPath = this.Initialize(Constants.MsiInstallerPathParameter, string.Empty);
            this.MsixInstallerPath = this.Initialize(Constants.MsixInstallerPathParameter, string.Empty);
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
        /// Gets or sets the cli path.
        /// </summary>
        public string AICLIPath { get; set; }

        /// <summary>
        /// Gets or sets the package path.
        /// </summary>
        public string AICLIPackagePath { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether the test runs in package context.
        /// </summary>
        public bool PackagedContext { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether the test uses verbose logging.
        /// </summary>
        public bool VerboseLogging { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether to use loose file registration.
        /// </summary>
        public bool LooseFileRegistration { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether to invoke command in desktop package.
        /// </summary>
        public bool InvokeCommandInDesktopPackage { get; set; }

        /// <summary>
        /// Gets or sets the static file root path.
        /// </summary>
        public string StaticFileRootPath { get; set; }

        /// <summary>
        /// Gets or sets the local server cert path.
        /// </summary>
        public string LocalServerCertPath { get; set; }

        /// <summary>
        /// Gets or sets the exe installer path.
        /// </summary>
        public string ExeInstallerPath { get; set; }

        /// <summary>
        /// Gets or sets the msi installer path.
        /// </summary>
        public string MsiInstallerPath { get; set; }

        /// <summary>
        /// Gets or sets the msix installer path.
        /// </summary>
        public string MsixInstallerPath { get; set; }

        /// <summary>
        /// Gets or sets the zip installer path.
        /// </summary>
        public string ZipInstallerPath { get; set; }

        /// <summary>
        /// Gets or sets the package cert path.
        /// </summary>
        public string PackageCertificatePath { get; set; }

        /// <summary>
        /// Gets or sets the PowerShell module path.
        /// </summary>
        public string PowerShellModulePath { get; set; }

        /// <summary>
        /// Gets or sets the settings json path.
        /// </summary>
        public string SettingsJsonFilePath { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether is the default parameters.
        /// </summary>
        public bool IsDefault { get; set; }

        private bool Initialize(string paramName, bool defaultValue)
        {
            if (this.IsDefault || !TestContext.Parameters.Exists(paramName))
            {
                return defaultValue;
            }

            return TestContext.Parameters.Get(paramName).Equals("true", StringComparison.OrdinalIgnoreCase);
        }

        private string Initialize(string paramName, string defaultVaule)
        {
            if (this.IsDefault || !TestContext.Parameters.Exists(paramName))
            {
                return defaultVaule;
            }

            return TestContext.Parameters.Get(paramName);
        }
    }
}
