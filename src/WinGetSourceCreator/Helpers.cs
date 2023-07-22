// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace Microsoft.WinGetSourceCreator
{
    using global::WinGetSourceCreator.Model;
    using Microsoft.Msix.Utils.ProcessRunner;
    using System.Diagnostics;
    using System.Xml;

    internal static class Helpers
    {
        public static void SignInstaller(SourceInstaller installer, Signature signature)
        {
            if (installer.Type == InstallerType.Msix)
            {
                SignMsixFile(installer.InstallerFile, signature);
            }
            else
            {
                SignFile(installer.InstallerFile, signature);
            }
        }

        public static void SignFile(string fileToSign, Signature signature)
        {
            if (!File.Exists(fileToSign))
            {
                throw new FileNotFoundException(fileToSign);
            }

            if (!File.Exists(signature.CertFile))
            {
                throw new FileNotFoundException(signature.CertFile);
            }

            string pathToSDK = SDKDetector.Instance.LatestSDKBinPath;
            string signtoolExecutable = Path.Combine(pathToSDK, "signtool.exe");
            string command = $"sign /a /fd sha256 /f {signature.CertFile} ";
            if (!string.IsNullOrEmpty(signature.Password))
            {
                command += $"/p {signature.Password} ";
            }
            command += fileToSign;
            RunCommand(signtoolExecutable, command);
        }

        public static void SignMsixFile(string fileToSign, Signature signature)
        {
            if (!File.Exists(fileToSign))
            {
                throw new FileNotFoundException(fileToSign);
            }

            // Modify publisher if needed.
            if (signature.Publisher != null)
            {
                string tmpPath = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString());
                Unpack(fileToSign, tmpPath);
                ModifyAppxManifestIdentity(Path.Combine(tmpPath, "AppxManifest.xml"), signature.Publisher);
                Pack(fileToSign, tmpPath);

                try
                {
                    Directory.Delete(tmpPath, true);
                }
                catch (Exception)
                {
                }
            }

            SignFile(fileToSign, signature);
        }

        public static void Unpack(string package, string outDir)
        {
            if (!File.Exists(package))
            {
                throw new FileNotFoundException(package);
            }

            string pathToSDK = SDKDetector.Instance.LatestSDKBinPath;
            string makeappxExecutable = Path.Combine(pathToSDK, "makeappx.exe");
            string args = $"unpack /nv /p {package} /d {outDir}";
            Process p = new Process
            {
                StartInfo = new ProcessStartInfo(makeappxExecutable, args)
            };
            p.Start();
            p.WaitForExit();
        }

        public static void PackWithMappingFile(string outputPackage, string mappingFile)
        {
            if (!File.Exists(mappingFile))
            {
                throw new FileNotFoundException(mappingFile);
            }

            string pathToSDK = SDKDetector.Instance.LatestSDKBinPath;
            string makeappxExecutable = Path.Combine(pathToSDK, "makeappx.exe");
            string args = $"pack /o /nv /f {mappingFile} /p {outputPackage}";
            RunCommand(makeappxExecutable, args);
        }

        public static void Pack(string outputPackage, string directoryToPack)
        {
            if (!Directory.Exists(directoryToPack))
            {
                throw new DirectoryNotFoundException(directoryToPack);
            }

            if (File.Exists(outputPackage))
            {
                File.Delete(outputPackage);
            }

            string pathToSDK = SDKDetector.Instance.LatestSDKBinPath;
            string makeappxExecutable = Path.Combine(pathToSDK, "makeappx.exe");
            string args = $"pack /o /d {directoryToPack} /p {outputPackage}";
            RunCommand(makeappxExecutable, args);
        }

        public static void RunCommand(string command, string args, string? workingDirectory = null)
        {
            Process p = new()
            {
                StartInfo = new ProcessStartInfo(command, args)
            };

            if (workingDirectory != null)
            {
                p.StartInfo.WorkingDirectory = workingDirectory;
            }
            p.Start();
            p.WaitForExit();
        }

        // If in the future we edit more elements, this should be a nice wrapper class.
        public static void ModifyAppxManifestIdentity(string manifestFile, string? identityPublisher)
        {
            if (!File.Exists(manifestFile))
            {
                throw new FileNotFoundException(manifestFile);
            }

            var xmlDoc = new XmlDocument();
            XmlNamespaceManager namespaces = new XmlNamespaceManager(xmlDoc.NameTable);
            namespaces.AddNamespace("n", "http://schemas.microsoft.com/appx/manifest/foundation/windows10");
            xmlDoc.Load(manifestFile);
            var identityNode = xmlDoc.SelectSingleNode("/n:Package/n:Identity", namespaces);
            if (identityNode == null)
            {
                throw new NullReferenceException("Identity node");
            }

            if (!string.IsNullOrEmpty(identityPublisher))
            {
                var attr = identityNode.Attributes?["Publisher"];
                if (attr == null)
                {
                    throw new NullReferenceException("Publisher attribute");
                }
                attr.Value = identityPublisher;
            }

            xmlDoc.Save(manifestFile);
        }

        // Gets the AppxSignature.p7x file.
        public static string GetSignatureFileFromMsix(string packageFilePath)
        {
            string extractedPackageDest = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString());
            if (Directory.Exists(extractedPackageDest))
            {
                Directory.Delete(extractedPackageDest, true);
            }

            Helpers.Unpack(packageFilePath, extractedPackageDest);

            return Path.Combine(extractedPackageDest, "AppxSignature.p7x");
        }

        public static void CopyDirectory(string sourceDirName, string destDirName)
        {
            if (!Directory.Exists(destDirName))
            {
                Directory.CreateDirectory(destDirName);
            }

            DirectoryInfo dir = new (sourceDirName);
            DirectoryInfo[] dirs = dir.GetDirectories();

            FileInfo[] files = dir.GetFiles();
            foreach (FileInfo file in files)
            {
                string temppath = Path.Combine(destDirName, file.Name);
                file.CopyTo(temppath, false);
            }

            foreach (DirectoryInfo subdir in dirs)
            {
                string temppath = Path.Combine(destDirName, subdir.Name);
                CopyDirectory(subdir.FullName, temppath);
            }
        }
    }
}
