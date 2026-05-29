﻿// -----------------------------------------------------------------------------
// <copyright file="WinGetUtilInstallerMetadataCollection.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests.WinGetUtil
{
    using System;
    using System.IO;
    using System.Runtime.InteropServices;
    using AppInstallerCLIE2ETests.Helpers;
    using Newtonsoft.Json;
    using NUnit.Framework;

    /// <summary>
    /// Test winget util installer metadata.
    /// </summary>
    public class WinGetUtilInstallerMetadataCollection
    {
        /// <summary>
        /// Test begin complete installer metadata.
        /// </summary>
        [Test]
        public void WinGetUtil_BeginCompleteInstallerMetadataCollection()
        {
            string logFilePath = TestCommon.GetRandomTestFile(".log");
            string inputJson = TestCommon.GetTestDataFile(@"WinGetUtil\InstallerMetadata\Minimal.json");
            string outputFilePath = TestCommon.GetRandomTestFile(".json");

            WinGetUtilWrapper.WinGetBeginInstallerMetadataCollection(
                inputJson,
                logFilePath,
                WinGetUtilWrapper.WinGetBeginInstallerMetadataCollectionOptions.WinGetBeginInstallerMetadataCollectionOption_InputIsFilePath,
                out IntPtr collectionHandle);

            Assert.That(collectionHandle, Is.Not.EqualTo(IntPtr.Zero));
            Assert.That(File.Exists(logFilePath), Is.True);

            WinGetUtilWrapper.WinGetCompleteInstallerMetadataCollection(
                collectionHandle,
                outputFilePath,
                WinGetUtilWrapper.WinGetCompleteInstallerMetadataCollectionOptions.WinGetCompleteInstallerMetadataCollectionOption_None);

            string outputJson = File.ReadAllText(outputFilePath);
            Assert.That(JsonConvert.DeserializeObject(outputJson).ToString(), Is.Not.Empty);
        }

        /// <summary>
        /// Test merge installer metadata.
        /// </summary>
        [Test]
        public void WinGetUtil_MergeInstallerMetadata_Success()
        {
            string logFilePath = TestCommon.GetRandomTestFile(".log");
            string inputJsonPath = TestCommon.GetTestDataFile(@"WinGetUtil\InstallerMetadata\MergeValid.json");
            string inputJson = File.ReadAllText(inputJsonPath);

            WinGetUtilWrapper.WinGetMergeInstallerMetadata(
               inputJson,
               out string outputJson,
               0,
               logFilePath,
               WinGetUtilWrapper.WinGetMergeInstallerMetadataOptions.WinGetMergeInstallerMetadataOptions_None);

            Assert.That(File.Exists(logFilePath), Is.True);
            Assert.That(JsonConvert.DeserializeObject(outputJson).ToString(), Is.Not.Empty);
        }

        /// <summary>
        /// Test merge installer metadata failed.
        /// </summary>
        [Test]
        public void WinGetUtil_MergeInstallerMetadata_Fail_SubmissionMismatch()
        {
            string logFilePath = TestCommon.GetRandomTestFile(".log");
            string inputJsonPath = TestCommon.GetTestDataFile(@"WinGetUtil\InstallerMetadata\MergeSubmissionMismatch.json");
            string inputJson = File.ReadAllText(inputJsonPath);

            Assert.That((Action)(() =>
            {
                WinGetUtilWrapper.WinGetMergeInstallerMetadata(
                   inputJson,
                   out string outputJson,
                   0,
                   logFilePath,
                   WinGetUtilWrapper.WinGetMergeInstallerMetadataOptions.WinGetMergeInstallerMetadataOptions_None);
            }), Throws.TypeOf<COMException>());

            Assert.That(File.Exists(logFilePath), Is.True);
        }
    }
}
