// -----------------------------------------------------------------------------
// <copyright file="TestIndex.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests.Helpers
{
    using System;
    using System.IO;
    using Microsoft.WinGetSourceCreator;
    using WinGetSourceCreator.Model;

    /// <summary>
    /// Test index setup.
    /// </summary>
    public static class TestIndex
    {
        /// <summary>
        /// Gets the signed exe installer path used by the manifests in the E2E test.
        /// </summary>
        public static string ExeInstaller { get; private set; }

        /// <summary>
        /// Gets the signed msi installer path used by the manifests in the E2E test.
        /// </summary>
        public static string MsiInstaller { get; private set; }

        /// <summary>
        /// Gets the signed msix installer path used by the manifests in the E2E test.
        /// </summary>
        public static string MsixInstaller { get; private set; }

        /// <summary>
        /// Gets the zip installer path used by the manifests in the E2E test.
        /// </summary>
        public static string ZipInstaller { get; private set; }

        /// <summary>
        /// Generate test source.
        /// </summary>
        public static void GenerateE2ESource()
        {
            var testParams = TestSetup.Parameters;

            LocalSource e2eSource = new ()
            {
                AppxManifest = Path.Combine(Environment.CurrentDirectory, "TestData", "Package", "AppxManifest.xml"),
                WorkingDirectory = testParams.StaticFileRootPath,
                LocalManifests = new ()
                {
                    Path.Combine(Environment.CurrentDirectory, "TestData", "Manifests"),
                },
                LocalInstallers = new ()
                {
                    new LocalInstaller
                    {
                        Type = InstallerType.Exe,
                        Name = Path.Combine(Constants.ExeInstaller, Constants.ExeInstallerFileName),
                        Input = testParams.ExeInstallerPath,
                        HashToken = "<EXEHASH>",
                    },
                    new LocalInstaller
                    {
                        Type = InstallerType.Msi,
                        Name = Path.Combine(Constants.MsiInstaller, Constants.MsiInstallerFileName),
                        Input = testParams.MsiInstallerPath,
                        HashToken = "<MSIHASH>",
                    },
                    new LocalInstaller
                    {
                        Type = InstallerType.Msix,
                        Name = Path.Combine(Constants.MsixInstaller, Constants.MsixInstallerFileName),
                        Input = testParams.MsixInstallerPath,
                        HashToken = "<MSIXHASH>",
                        SignatureToken = "<SIGNATUREHASH>",
                    },
                },
                DynamicInstallers = new ()
                {
                    new DynamicInstaller
                    {
                        Type = InstallerType.Zip,
                        Name = Path.Combine(Constants.ZipInstaller, Constants.ZipInstallerFileName),
                        Input = new ()
                        {
                            Path.Combine(testParams.StaticFileRootPath, Constants.ExeInstaller, Constants.ExeInstallerFileName),
                            Path.Combine(testParams.StaticFileRootPath, Constants.MsiInstaller, Constants.MsiInstallerFileName),
                            Path.Combine(testParams.StaticFileRootPath, Constants.MsixInstaller, Constants.MsixInstallerFileName),
                        },
                        HashToken = "<ZIPHASH>",
                    },
                },
                Signature = new ()
                {
                    CertFile = testParams.PackageCertificatePath,
                },
            };

            WinGetLocalSource.CreateLocalSource(e2eSource);

            // If everything goes right, modify the paths to the signed and final installers.
            TestIndex.ExeInstaller = Path.Combine(testParams.StaticFileRootPath, Constants.ExeInstaller, Constants.ExeInstallerFileName);
            TestIndex.MsiInstaller = Path.Combine(testParams.StaticFileRootPath, Constants.MsiInstaller, Constants.MsiInstallerFileName);
            TestIndex.MsixInstaller = Path.Combine(testParams.StaticFileRootPath, Constants.MsixInstaller, Constants.MsixInstallerFileName);
            TestIndex.ZipInstaller = Path.Combine(testParams.StaticFileRootPath, Constants.ZipInstaller, Constants.ZipInstallerFileName);
        }
    }
}
