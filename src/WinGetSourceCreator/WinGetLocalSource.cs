// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace Microsoft.WinGetSourceCreator
{
    using WinGetUtilInterop.Helpers;

    public class WinGetLocalSource
    {
        private readonly string workingDirectory;
        private readonly WinGetInstallerHashes installerHashes;

        public static void CreateLocalSource(LocalSource localSource)
        {
            localSource.Validate();

            var wingetSource = new WinGetLocalSource(localSource.WorkingDirectory);

            Signature? topSignature = localSource.Signature;
            if (localSource.LocalInstallers != null)
            {
                foreach (var localInstaller in localSource.LocalInstallers)
                {
                    var signature = localInstaller.Signature == null ? topSignature : localInstaller.Signature;
                    wingetSource.PrepareInstaller(localInstaller, signature);
                }
            }

            foreach (var localManifest in  localSource.LocalManifests)
            {
                wingetSource.PrepareManifest(localManifest);
            }

            var indexFile = wingetSource.CreateIndex(localSource.GetIndexName());

            _ = wingetSource.CreatePackage(localSource.GetSourceName(), localSource.AppxManifest, indexFile, topSignature);
        }

        public WinGetLocalSource(string workingDirectory)
        {
            this.workingDirectory = Path.GetFullPath(workingDirectory);

            if (Directory.Exists(workingDirectory))
            {
                Directory.Delete(workingDirectory, true);
            }
            Directory.CreateDirectory(workingDirectory);

            this.installerHashes = new();
        }

        public void PrepareInstaller(LocalInstaller localInstaller, Signature? signature)
        {
            var dest = Path.Combine(this.workingDirectory, Path.GetFileName(localInstaller.InstallerFile));
            if (localInstaller.IsMsix())
            {
                if (signature != null)
                {
                    Helpers.SignMsixFile(dest, signature);
                }
                else
                {
                    File.Copy(localInstaller.InstallerFile, dest, true);
                }

                this.installerHashes.AddMsix(localInstaller.InstallerFile, localInstaller.Token, localInstaller.MiscToken!);
            }
            else
            {
                File.Copy(localInstaller.InstallerFile, dest, true);
                if (signature != null)
                {
                    Helpers.SignFile(dest, signature.CertFile);
                }

                this.installerHashes.Add(localInstaller.InstallerFile, localInstaller.Token);
            }
        }

        public void PrepareManifest(string input)
        {
            if (File.Exists(input))
            {

                CopyManifestFile(input, Path.Combine(this.workingDirectory, Path.GetFileName(input)));
            }
            else
            {
                CopyManifestFiles(input, this.workingDirectory);
            }
            
        }

        public string CreateIndex(string indexName)
        {
            string fullPath = Path.Combine(this.workingDirectory, indexName);
            using var indexHelper = WinGetUtilIndex.CreateLatestVersion(fullPath);
            foreach (var file in Directory.EnumerateFiles(this.workingDirectory, "*.yaml", SearchOption.AllDirectories))
            {
                try
                {
                    var rel = Path.GetRelativePath(this.workingDirectory, file);
                    Console.WriteLine(rel);
                    indexHelper.AddManifest(file, Path.GetRelativePath(this.workingDirectory, file));
                }
                catch (Exception e)
                {
                    throw new InvalidOperationException($"Failed adding {file} to index", e);
                }
            }
            indexHelper.PrepareForPackaging();

            return fullPath;
        }

        public string CreatePackage(string packageName, string inputAppxManifestFile, string indexPath, Signature? signature)
        {
            if (!File.Exists(inputAppxManifestFile))
            {
                throw new FileNotFoundException(inputAppxManifestFile);
            }

            if (!File.Exists(indexPath))
            {
                throw new FileNotFoundException(indexPath);
            }

            string appxManifestFile = Path.Combine(this.workingDirectory, "AppxManifest.xml");
            File.Copy(inputAppxManifestFile, appxManifestFile);

            if (signature != null)
            {
                Helpers.ModifyAppxManifestIdentity(appxManifestFile, signature.Name, signature.Publisher);
            }

            string mappingFile = Path.Combine(this.workingDirectory, "MappingFile.txt");
            using StreamWriter outputFile = new(mappingFile);
            outputFile.WriteLine("[Files]");
            outputFile.WriteLine($"\"{Path.GetFileName(indexPath)}\" \"Public\\{Path.GetFileName(indexPath)}\"");
            outputFile.WriteLine($"\"{appxManifestFile}\" \"AppxManifest.xml\"");

            string outputPackage = Path.Combine(this.workingDirectory, packageName);
            Helpers.PackWithMappingFile(outputPackage, mappingFile);

            if (signature != null)
            {
                Helpers.SignFile(outputPackage, signature.CertFile);
            }

            return outputPackage;
        }

        // Copies all .yaml files
        private void CopyManifestFiles(string sourceDir, string destDir)
        {
            DirectoryInfo dir = new DirectoryInfo(sourceDir);
            DirectoryInfo[] dirs = dir.GetDirectories();

            FileInfo[] files = dir.GetFiles();
            foreach (FileInfo file in files)
            {
                if (file.Extension == ".yaml")
                {
                    CopyManifestFile(file.FullName, Path.Combine(destDir, file.Name));
                }
            }

            foreach (DirectoryInfo subdir in dirs)
            {
                CopyManifestFiles(subdir.FullName, Path.Combine(destDir, subdir.Name));
            }
        }

        // Copies a file and replaces any token found.
        private void CopyManifestFile(string sourceFile, string destinationFile)
        {
            if (!File.Exists(sourceFile))
            {
                throw new FileNotFoundException(sourceFile);
            }

            var content = File.ReadAllText(sourceFile);

            foreach (var token in this.installerHashes.InstallerTokens)
            {
                if (content.Contains(token.Key))
                {
                    content = content.Replace(token.Key, token.Value);
                }
            }

            File.WriteAllText(destinationFile, content);
        }
    }
}
