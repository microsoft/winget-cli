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

        public void Create(string outputFile)
        {
            if (this.Type == InstallerType.Zip)
            {
                CreateZipInstaller(outputFile);
            }
            else
            {
                throw new NotImplementedException();
            }
        }

        private void CreateZipInstaller(string outputFile)
        {
            var tmpPath = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString());
            if (Directory.Exists(tmpPath))
            {
                Directory.Delete(tmpPath, true);
            }
            Directory.CreateDirectory(tmpPath);

            foreach (var i in this.Input)
            {
                if (File.Exists(i))
                {
                    File.Copy(i, Path.Combine(tmpPath, Path.GetFileName(i)), true);
                }
                else if (Directory.Exists(i))
                {
                    Helpers.CopyDirectory(i, tmpPath);
                }
                else
                {
                    throw new InvalidOperationException(i);
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
