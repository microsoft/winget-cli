// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace WinGetSourceCreator.Model
{
    using Microsoft.WinGetSourceCreator;
    using System.IO.Compression;

    public class DynamicInstaller : Installer
    {
        // Input depends on the Type.
        // For zip it is the directories or files that need to included in the zip
        public List<string> Input { get; set; } = new List<string>();

        internal new void Validate()
        {
            base.Validate();
        }

        public string Create(string workingDirectory)
        {
            string outputFile = this.Name;
            if (!Path.IsPathFullyQualified(outputFile))
            {
                outputFile = Path.Combine(workingDirectory, outputFile);
            }

            var parent = Path.GetDirectoryName(outputFile);
            if (!string.IsNullOrEmpty(parent))
            {
                Directory.CreateDirectory(parent);
            }

            if (this.Type == InstallerType.Zip)
            {
                CreateZipInstaller(outputFile, workingDirectory);
            }
            else
            {
                throw new NotImplementedException();
            }

            return outputFile;
        }

        private void CreateZipInstaller(string outputFile, string workingDirectory)
        {
            var tmpPath = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString());
            if (Directory.Exists(tmpPath))
            {
                Directory.Delete(tmpPath, true);
            }
            Directory.CreateDirectory(tmpPath);

            foreach (var input in this.Input)
            {
                string fullPath = input;
                if (!Path.IsPathFullyQualified(fullPath))
                {
                    fullPath = Path.Combine(workingDirectory, fullPath);
                }

                if (File.Exists(fullPath))
                {
                    // TODO: maybe we want to preserve the dir?
                    File.Copy(fullPath, Path.Combine(tmpPath, Path.GetFileName(fullPath)), true);
                }
                else if (Directory.Exists(fullPath))
                {
                    Helpers.CopyDirectory(fullPath, tmpPath);
                }
                else
                {
                    throw new InvalidOperationException(fullPath);
                }
            }

            ZipFile.CreateFromDirectory(tmpPath, outputFile);

            try
            {
                Directory.Delete(tmpPath, true);
            }
            catch (Exception)
            {
            }
        }
    }
}
