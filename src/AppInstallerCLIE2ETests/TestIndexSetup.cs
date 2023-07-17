// -----------------------------------------------------------------------------
// <copyright file="TestIndexSetup.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using System;
    using System.IO;
    using Microsoft.WinGetSourceCreator;
    using WinGetSourceCreator.Model;

    /// <summary>
    /// Test index setup.
    /// </summary>
    public static class TestIndexSetup
    {
        /// <summary>
        /// Generate test source.
        /// </summary>
        public static void GenerateE2ESource()
        {
            LocalSource e2eSource = new ()
            {
                Name = "e2eSource",
                AppxManifest = Path.Combine(Environment.CurrentDirectory, "TestData", "Package", "AppxManifest.xml"),
                WorkingDirectory = TestCommon.StaticFileRootPath,
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
                        Input = TestCommon.ExeInstallerPath,
                        HashToken = "<EXEHASH>",
                    },
                    new LocalInstaller
                    {
                        Type = InstallerType.Msi,
                        Name = Path.Combine(Constants.MsiInstaller, Constants.MsiInstallerFileName),
                        Input = TestCommon.MsiInstallerPath,
                        HashToken = "<MSIHASH>",
                    },
                    new LocalInstaller
                    {
                        Type = InstallerType.Msix,
                        Name = Path.Combine(Constants.MsixInstaller, Constants.MsixInstallerFileName),
                        Input = TestCommon.MsixInstallerPath,
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
                            TestCommon.ExeInstallerPath,
                            TestCommon.MsiInstallerPath,
                            TestCommon.MsixInstallerPath,
                        },
                        HashToken = "<ZIPHASH>",
                    },
                },
                Signature = new ()
                {
                    CertFile = TestCommon.PackageCertificatePath,
                },
            };

            WinGetLocalSource.CreateLocalSource(e2eSource);
        }
    }
}
