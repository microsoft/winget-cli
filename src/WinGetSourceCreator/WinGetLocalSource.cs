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

            if (localSource.Delta != null)
            {
                wingetSource.CreateDeltaSource(localSource, localSource.Delta);
            }
        }

        // Publishes a second, delta capable source: a baseline, the full index, and the delta that
        // carries the difference between them.
        //
        // The baseline deliberately omits some manifests. If it held everything then the delta
        // would be empty, and a client that silently ignored it would look exactly like one that
        // merged it correctly.
        public void CreateDeltaSource(LocalSource localSource, DeltaSettings delta)
        {
            // Where the packages are published, and so what the source's URL resolves to. Preparing
            // an index also emits a per package version data file under this directory, which the
            // client fetches relative to that URL for anything beyond a search.
            string publishDirectory = Path.Combine(this.workingDirectory, delta.RelativeDirectory);

            // The working databases. They are never named by a package, but they do sit under the
            // static file root and so are reachable; that is already true of the ordinary source's
            // index.db and costs nothing here.
            string indexDirectory = Path.Combine(this.workingDirectory, "delta_indexes");
            string workingIndexPath = Path.Combine(indexDirectory, "working.db");
            string baselineIndexPath = Path.Combine(indexDirectory, "baseline.db");
            string fullIndexPath = Path.Combine(indexDirectory, "full.db");
            string deltaIndexPath = Path.Combine(indexDirectory, "delta.db");

            // Produced by designating the baseline, and of no use to anyone: an index that is its
            // own baseline has nothing to say.
            string discardedDeltaPath = Path.Combine(indexDirectory, "baseline.delta.db");

            if (Directory.Exists(indexDirectory))
            {
                Directory.Delete(indexDirectory, true);
            }

            Directory.CreateDirectory(indexDirectory);

            var excluded = new HashSet<string>(delta.ManifestsNotInBaseline, StringComparer.OrdinalIgnoreCase);
            var allManifests = Directory.EnumerateFiles(this.workingDirectory, "*.yaml", SearchOption.AllDirectories).ToList();
            var baselineManifests = allManifests.Where(file => !excluded.Contains(Path.GetFileName(file))).ToList();

            if (baselineManifests.Count == allManifests.Count)
            {
                throw new InvalidOperationException("None of the manifests named by ManifestsNotInBaseline were found, so the delta would be empty.");
            }

            WinGetFactory factory = new ();

            // The baseline and the full index are both prepared from copies of one working index.
            // That is not merely convenient: generation refuses a baseline whose database identifier
            // is not the source's, and the delta's window is the change sequence that the baseline
            // recorded, so two independently built indexes could not be paired at all.
            using (IWinGetSQLiteIndex workingIndex = factory.SQLiteIndexCreate(workingIndexPath, 2, 1))
            {
                this.AddManifestsToIndex(workingIndex, baselineManifests);
            }

            File.Copy(workingIndexPath, baselineIndexPath);
            using (IWinGetSQLiteIndex baselineIndex = factory.SQLiteIndexOpen(baselineIndexPath))
            {
                baselineIndex.SetProperty(SQLiteIndexProperty.IntermediateFileOutputPath, publishDirectory);
                baselineIndex.SetProperty(SQLiteIndexProperty.DeltaMarkAsBaseline, "true");
                baselineIndex.SetProperty(SQLiteIndexProperty.DeltaOutputPath, discardedDeltaPath);
                baselineIndex.SetProperty(SQLiteIndexProperty.DeltaBaselineRelativeSourcePath, delta.BaselineRelativePath);
                baselineIndex.SetProperty(SQLiteIndexProperty.DeltaBaselinePackageVersion, delta.BaselineVersion);
                baselineIndex.PrepareForPackaging();
            }

            // The manifests the baseline was built without become the delta's content.
            using (IWinGetSQLiteIndex workingIndex = factory.SQLiteIndexOpen(workingIndexPath))
            {
                this.AddManifestsToIndex(workingIndex, allManifests.Except(baselineManifests));
            }

            File.Copy(workingIndexPath, fullIndexPath);
            using (IWinGetSQLiteIndex fullIndex = factory.SQLiteIndexOpen(fullIndexPath))
            {
                fullIndex.SetProperty(SQLiteIndexProperty.IntermediateFileOutputPath, publishDirectory);
                fullIndex.SetProperty(SQLiteIndexProperty.DeltaBaselineIndexPath, baselineIndexPath);
                fullIndex.SetProperty(SQLiteIndexProperty.DeltaOutputPath, deltaIndexPath);
                fullIndex.SetProperty(SQLiteIndexProperty.DeltaBaselineRelativeSourcePath, delta.BaselineRelativePath);
                fullIndex.SetProperty(SQLiteIndexProperty.DeltaBaselinePackageVersion, delta.BaselineVersion);
                fullIndex.PrepareForPackaging();
            }

            if (!File.Exists(deltaIndexPath))
            {
                throw new InvalidOperationException($"Preparing the full index did not produce a delta at {deltaIndexPath}");
            }

            // Every package names the index as "index.db" regardless of the working file it came
            // from, since that is the only name a client looks for.
            string indexNameInPackage = localSource.GetIndexName();

            // The relative path is recorded in the delta as a URL fragment, so it keeps its forward
            // slashes there; only the on disk location needs separators this machine understands.
            string baselinePackagePath = Path.Combine(
                delta.RelativeDirectory,
                delta.BaselineRelativePath.Replace('/', Path.DirectorySeparatorChar));

            _ = this.CreatePackage(
                baselinePackagePath,
                localSource.AppxManifest,
                baselineIndexPath,
                localSource.Signature,
                delta.BaselineIdentityName,
                delta.BaselineVersion,
                indexNameInPackage);

            _ = this.CreatePackage(
                Path.Combine(delta.RelativeDirectory, localSource.GetSourceName(2)),
                localSource.AppxManifest,
                fullIndexPath,
                localSource.Signature,
                delta.IdentityName,
                delta.Version,
                indexNameInPackage);

            _ = this.CreatePackage(
                Path.Combine(delta.RelativeDirectory, "delta.msix"),
                localSource.AppxManifest,
                deltaIndexPath,
                localSource.Signature,
                delta.DeltaIdentityName,
                delta.Version,
                indexNameInPackage);
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

            this.AddManifestsToIndex(indexHelper, Directory.EnumerateFiles(this.workingDirectory, "*.yaml", SearchOption.AllDirectories));

            indexHelper.PrepareForPackaging();

            return fullPath;
        }

        // Adds every given manifest, retrying the ones that fail. A manifest can legitimately fail
        // on the first attempt when it names a package dependency that is not in the index yet.
        private void AddManifestsToIndex(IWinGetSQLiteIndex indexHelper, IEnumerable<string> manifestFiles)
        {
            Queue<string> filesQueue = new(manifestFiles);
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
        }

        public string CreatePackage(
            string packageName,
            string inputAppxManifestFile,
            string indexPath,
            Signature? signature,
            string? identityName = null,
            string? identityVersion = null,
            string? indexNameInPackage = null)
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

            if ((signature != null && signature.Publisher != null) || identityName != null || identityVersion != null)
            {
                Helpers.ModifyAppxManifestIdentity(appxManifestFile, signature?.Publisher, identityName, identityVersion);
            }

            string mappingFile = Path.Combine(this.workingDirectory, "MappingFile.txt");

            {
                using StreamWriter outputFile = new(mappingFile, false);
                outputFile.WriteLine("[Files]");
                outputFile.WriteLine($"\"{indexPath}\" \"Public\\{indexNameInPackage ?? Path.GetFileName(indexPath)}\"");
                outputFile.WriteLine($"\"{appxManifestFile}\" \"AppxManifest.xml\"");
            }

            string outputPackage = Path.Combine(this.workingDirectory, packageName);

            string? outputDirectory = Path.GetDirectoryName(outputPackage);
            if (!string.IsNullOrEmpty(outputDirectory))
            {
                Directory.CreateDirectory(outputDirectory);
            }

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
