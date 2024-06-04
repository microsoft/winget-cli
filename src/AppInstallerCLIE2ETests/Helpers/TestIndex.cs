// -----------------------------------------------------------------------------
// <copyright file="TestIndex.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests.Helpers
{
    using System;
    using System.IO;
    using System.Text.Json;
    using Microsoft.WinGetSourceCreator;
    using WinGetSourceCreator.Model;

    /// <summary>
    /// Test index setup.
    /// </summary>
    public static class TestIndex
    {
        static TestIndex()
        {
            // Expected path for the installers.
            TestIndex.ExeInstaller = Path.Combine(TestSetup.Parameters.StaticFileRootPath, Constants.ExeInstaller, Constants.ExeInstallerFileName);
            TestIndex.MsiInstaller = Path.Combine(TestSetup.Parameters.StaticFileRootPath, Constants.MsiInstaller, Constants.MsiInstallerFileName);
            TestIndex.MsiInstallerV2 = Path.Combine(TestSetup.Parameters.StaticFileRootPath, Constants.MsiInstaller, Constants.MsiInstallerV2FileName);
            TestIndex.MsixInstaller = Path.Combine(TestSetup.Parameters.StaticFileRootPath, Constants.MsixInstaller, Constants.MsixInstallerFileName);
            TestIndex.ZipInstaller = Path.Combine(TestSetup.Parameters.StaticFileRootPath, Constants.ZipInstaller, Constants.ZipInstallerFileName);
        }

        /// <summary>
        /// Gets the signed exe installer path used by the manifests in the E2E test.
        /// </summary>
        public static string ExeInstaller { get; private set; }

        /// <summary>
        /// Gets the signed msi installer path used by the manifests in the E2E test.
        /// </summary>
        public static string MsiInstaller { get; private set; }

        /// <summary>
        /// Gets the signed msi installerV2 path used by the manifests in the E2E test.
        /// </summary>
        public static string MsiInstallerV2 { get; private set; }

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

            if (string.IsNullOrEmpty(testParams.ExeInstallerPath))
            {
                throw new ArgumentNullException($"{Constants.ExeInstallerPathParameter} is required");
            }

            if (!File.Exists(testParams.ExeInstallerPath))
            {
                throw new FileNotFoundException(testParams.ExeInstallerPath);
            }

            if (string.IsNullOrEmpty(testParams.MsiInstallerPath))
            {
                throw new ArgumentNullException($"{Constants.MsiInstallerPathParameter} is required");
            }

            if (!File.Exists(testParams.MsiInstallerPath))
            {
                throw new FileNotFoundException(testParams.MsiInstallerPath);
            }

            if (string.IsNullOrEmpty(testParams.MsiInstallerV2Path))
            {
                throw new ArgumentNullException($"{Constants.MsiInstallerV2PathParameter} is required");
            }

            if (!File.Exists(testParams.MsiInstallerV2Path))
            {
                throw new FileNotFoundException(testParams.MsiInstallerV2Path);
            }

            if (string.IsNullOrEmpty(testParams.MsixInstallerPath))
            {
                throw new ArgumentNullException($"{Constants.MsixInstallerPathParameter} is required");
            }

            if (!File.Exists(testParams.MsixInstallerPath))
            {
                throw new FileNotFoundException(testParams.MsixInstallerPath);
            }

            if (string.IsNullOrEmpty(testParams.PackageCertificatePath))
            {
                throw new ArgumentNullException($"{Constants.PackageCertificatePathParameter} is required");
            }

            if (!File.Exists(testParams.PackageCertificatePath))
            {
                throw new FileNotFoundException(testParams.PackageCertificatePath);
            }

            LocalSource e2eSource = new ()
            {
                AppxManifest = TestCommon.GetTestDataFile(Path.Combine("Package", "AppxManifest.xml")),
                WorkingDirectory = testParams.StaticFileRootPath,
                LocalManifests = new ()
                {
                    TestCommon.GetTestDataFile("Manifests"),
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
                        Type = InstallerType.Msi,
                        Name = Path.Combine(Constants.MsiInstaller, Constants.MsiInstallerV2FileName),
                        Input = testParams.MsiInstallerPath,
                        HashToken = "<MSIHASHV2>",
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
                            ExeInstaller,
                            MsiInstaller,
                            MsixInstaller,
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
        }
    }
}
