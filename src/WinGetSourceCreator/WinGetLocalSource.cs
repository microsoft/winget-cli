// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace Microsoft.WinGetSourceCreator
{
    using global::WinGetSourceCreator.Model;
    using System.Text.Json.Serialization;
    using System.Text.Json;
    using Microsoft.WinGetUtil.Api;
    using Microsoft.WinGetUtil.Interfaces;

    public class WinGetLocalSource
    {
        private readonly string workingDirectory;
        private readonly ManifestTokens tokens;
        private readonly Signature? signature;

        public static void CreateFromLocalSourceFile(string localSourceFile)
        {
            var content = File.ReadAllText(localSourceFile);
            content = Environment.ExpandEnvironmentVariables(content);

            var options = new JsonSerializerOptions
            {
                PropertyNameCaseInsensitive = true,
                Converters =
                {
                    new JsonStringEnumConverter(JsonNamingPolicy.CamelCase)
                }
            };

            content = content.Replace("\\", "/");

            var localSource = JsonSerializer.Deserialize<LocalSource>(content, options);
            if (localSource == null)
            {
                throw new Exception("Failed deserializing");
            }

            CreateLocalSource(localSource);
        }

        public static void CreateLocalSource(LocalSource localSource)
        {
            localSource.Validate();

            var wingetSource = new WinGetLocalSource(localSource.WorkingDirectory, localSource.Signature);

            if (localSource.LocalInstallers != null)
            {
                foreach (var installer in localSource.LocalInstallers)
                {
                    wingetSource.PrepareLocalInstaller(installer);
                }
            }

            if (localSource.DynamicInstallers != null)
            {
                foreach (var installer in localSource.DynamicInstallers)
                {
                    wingetSource.PrepareDynamicInstaller(installer);
                }
            }

            foreach (var localManifest in localSource.LocalManifests)
            {
                wingetSource.PrepareManifest(localManifest);
            }

            var indexV2File = wingetSource.CreateIndex(localSource.GetIndexName(), 2, 0);
            _ = wingetSource.CreatePackage(localSource.GetSourceName(2), localSource.AppxManifest, indexV2File, localSource.Signature);

            var indexV1File = wingetSource.CreateIndex(localSource.GetIndexName());
            _ = wingetSource.CreatePackage(localSource.GetSourceName(1), localSource.AppxManifest, indexV1File, localSource.Signature);
        }

        public WinGetLocalSource(string workingDirectory, Signature? signature)
        {
            this.workingDirectory = Path.GetFullPath(workingDirectory);

            if (Directory.Exists(workingDirectory))
            {
                Directory.Delete(workingDirectory, true);
            }
            Directory.CreateDirectory(workingDirectory);

            this.tokens = new();
            this.signature = signature;
        }

        public void PrepareDynamicInstaller(DynamicInstaller installer)
        {
            var sourceInstaller = new SourceInstaller(this.workingDirectory, installer);
            PrepareInstaller(sourceInstaller);
        }

        public void PrepareLocalInstaller(LocalInstaller installer)
        {
            var sourceInstaller = new SourceInstaller(this.workingDirectory, installer);
            PrepareInstaller(sourceInstaller);
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

        public string CreateIndex(string indexName, uint? majorVersion = null, uint? minorVersion = null)
        {
            string fullPath = Path.Combine(this.workingDirectory, indexName);

            if (File.Exists(fullPath))
            {
                File.Delete(fullPath);
            }

            WinGetFactory factory = new ();
            using IWinGetSQLiteIndex indexHelper = majorVersion == null ? factory.SQLiteIndexCreateLatestVersion(fullPath) : factory.SQLiteIndexCreate(fullPath, majorVersion.Value, minorVersion.GetValueOrDefault());

            Queue<string> filesQueue = new(Directory.EnumerateFiles(this.workingDirectory, "*.yaml", SearchOption.AllDirectories));
            while (filesQueue.Count > 0)
            {
                int currentCount = filesQueue.Count;

                for (int i = 0; i < currentCount; i++)
                {
                    string file = filesQueue.Dequeue();
                    try
                    {
                        var rel = Path.GetRelativePath(this.workingDirectory, file);
                        indexHelper.AddManifest(file, rel);
                    }
                    catch
                    {
                        // If adding manifest to index fails, add to queue and try again.
                        // This can occur if there is a package dependency that has not yet been added to the index.
                        filesQueue.Enqueue(file);
                    }
                }

                if (filesQueue.Count == currentCount)
                {
                    throw new InvalidOperationException("Failed to add all manifests in directory to index.");
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
            File.Copy(inputAppxManifestFile, appxManifestFile, true);

            if (signature != null && signature.Publisher != null)
            {
                Helpers.ModifyAppxManifestIdentity(appxManifestFile, signature.Publisher);
            }

            string mappingFile = Path.Combine(this.workingDirectory, "MappingFile.txt");

            {
                using StreamWriter outputFile = new(mappingFile, false);
                outputFile.WriteLine("[Files]");
                outputFile.WriteLine($"\"{indexPath}\" \"Public\\{Path.GetFileName(indexPath)}\"");
                outputFile.WriteLine($"\"{appxManifestFile}\" \"AppxManifest.xml\"");
            }

            string outputPackage = Path.Combine(this.workingDirectory, packageName);
            Helpers.PackWithMappingFile(outputPackage, mappingFile);

            if (signature != null)
            {
                Helpers.SignFile(outputPackage, signature);
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

            foreach (var token in this.tokens.Tokens)
            {
                if (content.Contains(token.Key))
                {
                    content = content.Replace(token.Key, token.Value);
                }
            }

            File.WriteAllText(destinationFile, content);
        }

        private void PrepareInstaller(SourceInstaller installer)
        {
            // Sign installer if needed.
            if (!installer.SkipSignature)
            {
                var sig = this.GetSignature(installer);
                if (sig != null)
                {
                    Helpers.SignInstaller(installer, sig);
                }
            }

            // Process hash token if needed.
            if (!string.IsNullOrEmpty(installer.HashToken))
            {
                this.tokens.AddHashToken(installer.InstallerFile, installer.HashToken);
            }

            // Extra steps.
            // An msix can include the signature token.
            if (installer.Type == InstallerType.Msix)
            {
                if (!string.IsNullOrEmpty(installer.SignatureToken))
                {
                    var signatureFilePath = Helpers.GetSignatureFileFromMsix(installer.InstallerFile);
                    this.tokens.AddHashToken(signatureFilePath, installer.SignatureToken);

                    try
                    {
                        var dir = Path.GetDirectoryName(signatureFilePath);
                        if (!string.IsNullOrEmpty(dir))
                        {
                            Directory.Delete(dir, true);
                        }
                    }
                    catch (Exception)
                    {
                    }
                }
            }
        }

        private Signature? GetSignature(Installer installer)
        {
            if (installer.Type == InstallerType.Zip)
            {
                return null;
            }

            return installer.Signature == null ? this.signature : installer.Signature;
        }
    }
}
