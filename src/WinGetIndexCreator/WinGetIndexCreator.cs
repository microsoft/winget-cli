// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace Microsoft.WinGetSourceCreator
{
    using WinGetUtilInterop.Helpers;

    internal class WinGetIndexCreator
    {
        private const string Manifests = "manifests";
        private readonly string workingDirectory;

        public WinGetIndexCreator(string workingDirectory)
        {
            this.workingDirectory = Path.GetFullPath(workingDirectory);
        }

        public void PrepareManifests(string sourceDir, Dictionary<string, string>? replaceTokens = null)
        {
            string manifestsPath = Path.Combine(this.workingDirectory, Manifests);
            Directory.CreateDirectory(manifestsPath);

            CopyManifestFiles(sourceDir, manifestsPath, replaceTokens);
        }

        /// <summary>
        /// Creates an index looking at all the yaml files under a "manifest" directory.
        /// </summary>
        /// <param name="indexName">Index name.</param>
        /// <returns>True if all files were added.</returns>
        public bool CreateIndex(string indexName)
        {
            bool includedAllFiles = true;
            string manifestsPath = Path.Combine(this.workingDirectory, "manifests");
            using var indexHelper = WinGetUtilIndex.CreateLatestVersion(indexName);
            foreach (var file in Directory.EnumerateFiles(manifestsPath, "*.yaml", SearchOption.AllDirectories))
            {
                try
                {
                    indexHelper.AddManifest(file, Path.GetRelativePath(this.workingDirectory, manifestsPath));
                }
                catch (Exception)
                {
                    includedAllFiles = false;
                }
            }
            indexHelper.PrepareForPackaging();
            return includedAllFiles;
        }

        /// <summary>
        /// Copies all .yaml files
        /// </summary>
        /// <param name="sourceDir">Source directory</param>
        /// <param name="destDir">Destination directory.</param>
        private void CopyManifestFiles(string sourceDir, string destDir, Dictionary<string, string>? replaceTokens = null)
        {
            DirectoryInfo dir = new DirectoryInfo(sourceDir);
            DirectoryInfo[] dirs = dir.GetDirectories();

            FileInfo[] files = dir.GetFiles();
            foreach (FileInfo file in files)
            {
                if (file.Extension == ".yaml")
                {
                    CopyManifestFile(file.FullName, Path.Combine(destDir, file.Name), replaceTokens);
                }
            }

            foreach (DirectoryInfo subdir in dirs)
            {
                CopyManifestFiles(subdir.FullName, Path.Combine(destDir, subdir.Name));
            }
        }

        /// <summary>
        /// Copies a file and replaces any token found.
        /// </summary>
        /// <param name="sourceFile">Source file.</param>
        /// <param name="destinationFile">Destination file.</param>
        /// <param name="replaceTokens">Optional tokens to replace.</param>
        private void CopyManifestFile(string sourceFile, string destinationFile, Dictionary<string, string>? replaceTokens = null)
        {
            if (!File.Exists(sourceFile))
            {
                throw new FileNotFoundException(sourceFile);
            }

            var content = File.ReadAllText(sourceFile);

            if (replaceTokens != null)
            {
                foreach (var token in replaceTokens)
                {
                    if (content.Contains(token.Key))
                    {
                        content.Replace(token.Key, token.Value);
                    }
                }
            }

            File.WriteAllText(destinationFile, content);
        }

    }
}