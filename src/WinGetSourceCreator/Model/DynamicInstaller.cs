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
                CreateZipInstaller(outputFile);
            }
            else
            {
                throw new NotImplementedException();
            }

            return outputFile;
        }

        private void CreateZipInstaller(string outputFile)
        {
            var tmpPath = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString());
            if (Directory.Exists(tmpPath))
            {
                Directory.Delete(tmpPath, true);
            }
            Directory.CreateDirectory(tmpPath);

            foreach (var input in this.Input)
            {
                if (!Path.IsPathFullyQualified(input))
                {
                    throw new InvalidOperationException($"Must be a fully qualified name {input}");
                }

                if (File.Exists(input))
                {
                    // TODO: maybe we want to preserve the dir?
                    File.Copy(input, Path.Combine(tmpPath, Path.GetFileName(input)), true);
                }
                else if (Directory.Exists(input))
                {
                    Helpers.CopyDirectory(input, tmpPath);
                }
                else
                {
                    throw new InvalidOperationException(input);
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
