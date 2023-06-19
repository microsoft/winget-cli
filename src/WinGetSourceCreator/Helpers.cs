// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace Microsoft.WinGetSourceCreator
{
    using Microsoft.Msix.Utils.ProcessRunner;
    using System.Diagnostics;
    using System.Xml;

    internal static class Helpers
    {
        public static void SignFile(string filePath, string certFile)
        {
            if (!File.Exists(filePath))
            {
                throw new FileNotFoundException(filePath);
            }

            if (!File.Exists(certFile))
            {
                throw new FileNotFoundException(certFile);
            }

            string pathToSDK = SDKDetector.Instance.LatestSDKBinPath;
            string signtoolExecutable = Path.Combine(pathToSDK, "signtool.exe");
            RunCommand(signtoolExecutable, $"sign /a /fd sha256 /f {certFile} {filePath}");
        }

        public static void SignMsixFile(string filePath, Signature signature)
        {
            if (!File.Exists(filePath))
            {
                throw new FileNotFoundException(filePath);
            }

            if (!File.Exists(signature.CertFile))
            {
                throw new FileNotFoundException(signature.CertFile);
            }

            // If any of these are set modify the manifest and pack it.
            if (signature.Name != null || signature.Publisher != null)
            {
                string tmpPath = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString());
                Unpack(filePath, tmpPath);
                ModifyAppxManifestIdentity(Path.Combine(tmpPath, "AppxManifest.xml"), signature.Name, signature.Publisher);
                Pack(filePath, tmpPath);

                try
                {
                    Directory.Delete(tmpPath, true);
                }
                catch (Exception)
                {
                }
            }

            string pathToSDK = SDKDetector.Instance.LatestSDKBinPath;
            string signtoolExecutable = Path.Combine(pathToSDK, "signtool.exe");
            RunCommand(signtoolExecutable, $"sign /a /fd sha256 /f {signature.CertFile} {filePath}");
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
            string args = $"pack /o /f {mappingFile} /d {outputPackage}";
            Process p = new()
            {
                StartInfo = new ProcessStartInfo(makeappxExecutable, args)
            };
            p.Start();
            p.WaitForExit();
        }

        public static void Pack(string outputPackage, string directoryToPack)
        {
            if (!Directory.Exists(directoryToPack))
            {
                throw new DirectoryNotFoundException(directoryToPack);
            }

            if (!File.Exists(outputPackage))
            {
                File.Delete(outputPackage);
            }

            string pathToSDK = SDKDetector.Instance.LatestSDKBinPath;
            string makeappxExecutable = Path.Combine(pathToSDK, "makeappx.exe");
            string args = $"pack /o /d {directoryToPack} /d {outputPackage}";
            Process p = new()
            {
                StartInfo = new ProcessStartInfo(makeappxExecutable, args)
            };
            p.Start();
            p.WaitForExit();
        }

        public static void RunCommand(string command, string args, string? workingDirectory = null)
        {
            Process p = new Process();
            p.StartInfo = new ProcessStartInfo(command, args);

            if (workingDirectory != null)
            {
                p.StartInfo.WorkingDirectory = workingDirectory;
            }
            p.Start();
            p.WaitForExit();
        }

        // If in the future we edit more elements, this should be a nice wrapper class.
        public static void ModifyAppxManifestIdentity(string manifestFile, string? identityName, string? identityPublisher)
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

            if (identityName != null)
            {
                var atr = identityNode.Attributes?["Name"];
                if (atr == null)
                {
                    throw new NullReferenceException("Name attribute");
                }
                atr.Value = identityName;
            }

            if (identityPublisher != null)
            {
                var atr = identityNode.Attributes?["Publisher"];
                if (atr == null)
                {
                    throw new NullReferenceException("Publisher attribute");
                }
                atr.Value = identityPublisher;
            }

            xmlDoc.Save(manifestFile);
        }
    }
}
