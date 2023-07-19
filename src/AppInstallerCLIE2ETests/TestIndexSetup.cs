﻿// -----------------------------------------------------------------------------
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
                            Path.Combine(TestCommon.StaticFileRootPath, Constants.ExeInstaller, Constants.ExeInstallerFileName),
                            Path.Combine(TestCommon.StaticFileRootPath, Constants.MsiInstaller, Constants.MsiInstallerFileName),
                            Path.Combine(TestCommon.StaticFileRootPath, Constants.MsixInstaller, Constants.MsixInstallerFileName),
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

            // If everything goes right, modify the paths to the signed and final installers.
            TestCommon.ExeInstallerPath = Path.Combine(TestCommon.StaticFileRootPath, Constants.ExeInstaller, Constants.ExeInstallerFileName);
            TestCommon.MsiInstallerPath = Path.Combine(TestCommon.StaticFileRootPath, Constants.MsiInstaller, Constants.MsiInstallerFileName);
            TestCommon.MsixInstallerPath = Path.Combine(TestCommon.StaticFileRootPath, Constants.MsixInstaller, Constants.MsixInstallerFileName);
            TestCommon.ZipInstallerPath = Path.Combine(TestCommon.StaticFileRootPath, Constants.ZipInstaller, Constants.ZipInstallerFileName);
        }
    }
}
