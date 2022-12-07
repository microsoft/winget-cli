// -----------------------------------------------------------------------------
// <copyright file="TestHashHelper.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using System;
    using System.IO;
    using System.Security.Cryptography;
    using Microsoft.Msix.Utils.ProcessRunner;

    /// <summary>
    /// TestHashHelper.
    /// </summary>
    public class TestHashHelper
    {
        /// <summary>
        /// Gets or sets the exe installer hash value.
        /// </summary>
        public static string ExeInstallerHashValue { get; set; }

        /// <summary>
        /// Gets or sets the msi installer hash value.
        /// </summary>
        public static string MsiInstallerHashValue { get; set; }

        /// <summary>
        /// Gets or sets the msix installer hash value.
        /// </summary>
        public static string MsixInstallerHashValue { get; set; }

        /// <summary>
        /// Gets or sets the zip installer hash value.
        /// </summary>
        public static string ZipInstallerHashValue { get; set; }

        /// <summary>
        /// Gets or sets the signature hash value.
        /// </summary>
        public static string SignatureHashValue { get; set; }

        /// <summary>
        /// Sets the hash of the installers.
        /// </summary>
        public static void HashInstallers()
        {
            if (!string.IsNullOrEmpty(TestCommon.ExeInstallerPath))
            {
                ExeInstallerHashValue = HashFile(TestCommon.ExeInstallerPath);
            }

            if (!string.IsNullOrEmpty(TestCommon.MsiInstallerPath))
            {
                MsiInstallerHashValue = HashFile(TestCommon.MsiInstallerPath);
            }

            if (!string.IsNullOrEmpty(TestCommon.MsixInstallerPath))
            {
                MsixInstallerHashValue = HashFile(TestCommon.MsixInstallerPath);
                SignatureHashValue = HashSignatureFromMSIX(TestCommon.MsixInstallerPath);
            }

            if (!string.IsNullOrEmpty(TestCommon.ZipInstallerPath))
            {
                ZipInstallerHashValue = HashFile(TestCommon.ZipInstallerPath);
            }
        }

        /// <summary>
        /// Iterates through all manifest files in a directory and replaces the hash token with the
        /// corresponding installer hash token.
        /// </summary>
        /// <param name="pathToManifestDir">Path to manifest directory.</param>
        public static void ReplaceManifestHashToken(string pathToManifestDir)
        {
            var dir = new DirectoryInfo(pathToManifestDir);
            FileInfo[] files = dir.GetFiles();

            foreach (FileInfo file in files)
            {
                string text = File.ReadAllText(file.FullName);

                if (text.Contains("<EXEHASH>"))
                {
                    text = text.Replace("<EXEHASH>", ExeInstallerHashValue);
                    File.WriteAllText(file.FullName, text);
                }
                else if (text.Contains("<MSIHASH>"))
                {
                    text = text.Replace("<MSIHASH>", MsiInstallerHashValue);
                    File.WriteAllText(file.FullName, text);
                }
                else if (text.Contains("<MSIXHASH>"))
                {
                    text = text.Replace("<MSIXHASH>", MsixInstallerHashValue);

                    if (text.Contains("<SIGNATUREHASH>"))
                    {
                        text = text.Replace("<SIGNATUREHASH>", SignatureHashValue);
                    }

                    File.WriteAllText(file.FullName, text);
                }
                else if (text.Contains("<ZIPHASH>"))
                {
                    text = text.Replace("<ZIPHASH>", ZipInstallerHashValue);
                    File.WriteAllText(file.FullName, text);
                }
            }
        }

        /// <summary>
        /// Gets hash of the AppxSignature.p7x file in the msix.
        /// </summary>
        /// <param name="packageFilePath">Package file path.</param>
        /// <returns>Hash of signature file.</returns>
        public static string HashSignatureFromMSIX(string packageFilePath)
        {
            // Obtain MakeAppX Executable Path
            string pathToSDK = SDKDetector.Instance.LatestSDKBinPath;
            string makeappxExecutable = Path.Combine(pathToSDK, "makeappx.exe");

            // Generate temp path to unpack MSIX package
            FileInfo fileInfo = new FileInfo(packageFilePath);
            string packageName = Path.GetFileNameWithoutExtension(fileInfo.Name);
            string tempPath = Path.GetTempPath();
            string extractedPackageDest = Path.Combine(tempPath, packageName);

            // Delete existing extracted package directories to avoid stalling MakeAppX command
            if (Directory.Exists(extractedPackageDest))
            {
                TestIndexSetup.DeleteDirectoryContents(Directory.CreateDirectory(extractedPackageDest));
                Directory.Delete(extractedPackageDest);
            }

            TestIndexSetup.RunCommand(makeappxExecutable, $"unpack /nv /p {packageFilePath} /d {extractedPackageDest}");

            string packageSignaturePath = Path.Combine(extractedPackageDest, "AppxSignature.p7x");
            return HashFile(packageSignaturePath);
        }

        /// <summary>
        /// Gets the hash of the specified file.
        /// </summary>
        /// <param name="filePath">File path.</param>
        /// <returns>Hash of file.</returns>
        public static string HashFile(string filePath)
        {
            FileInfo file;

            try
            {
                file = new FileInfo(filePath);
            }
            catch (FileNotFoundException e)
            {
                Console.WriteLine($"File Not Found: {e.Message}");
                throw;
            }

            string hash = string.Empty;

            using (SHA256 mySHA256 = SHA256.Create())
            {
                try
                {
                    FileStream fileStream = file.Open(FileMode.Open);
                    fileStream.Position = 0;
                    byte[] hashValue = mySHA256.ComputeHash(fileStream);
                    hash = ConvertHashByteToString(hashValue);
                    fileStream.Close();
                }
                catch (IOException e)
                {
                    Console.WriteLine($"I/O Exception: {e.Message}");
                    throw;
                }
                catch (UnauthorizedAccessException e)
                {
                    Console.WriteLine($"Access Exception: {e.Message}");
                    throw;
                }
            }

            return hash;
        }

        /// <summary>
        /// Converts the byte hash into its string format.
        /// </summary>
        /// <param name="array">Hash.</param>
        /// <returns>Hash as string.</returns>
        public static string ConvertHashByteToString(byte[] array)
        {
            string hashValue = string.Empty;

            for (int i = 0; i < array.Length; i++)
            {
                hashValue = hashValue + $"{array[i]:X2}";
            }

            return hashValue;
        }
    }
}
