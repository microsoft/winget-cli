using System;
using System.Collections.Generic;
using System.IO;
using System.Security.Cryptography;
using System.Text;

namespace AppInstallerCLIE2ETests
{
    public class TestHashHelper
    {
        public static string ExeInstallerHashValue { get; set; }

        public static string MsiInstallerHashValue { get; set; }

        public static string MsixInstallerHashValue { get; set; }

        public static void HashInstallers()
        {
            ExeInstallerHashValue = HashInstallerFile(TestCommon.ExeInstallerPath);
            //MsiInstallerHashValue = HashInstallerFile(TestCommon.MsiInstallerPath);
            //MsixInstallerHashValue = HashInstallerFile(TestCommon.MsixInstallerPath);
        }

        /// <summary>
        /// Iterates through all manifest files in a directory and replaces the hash <token> with the
        /// corresponding installer hash token.
        /// </summary>
        /// <param name="pathToManifestDir"></param>        
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
                    File.WriteAllText(file.FullName, text);
                }
            }
        }

        public static string HashInstallerFile(string installerFilePath)
        {
            FileInfo installerFile = new FileInfo(installerFilePath);
            string hash = string.Empty;

            using (SHA256 mySHA256 = SHA256.Create())
            {
                try
                {
                    FileStream fileStream = installerFile.Open(FileMode.Open);
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
