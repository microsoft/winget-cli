// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace Microsoft.WinGetSourceCreator
{
    public class WinGetLocalSource
    {
        private readonly string workingDirectory;
        private readonly WinGetInstallerHashes installerHashes;

        public WinGetLocalSource(string workingDirectory)
        {
            if (Directory.Exists(workingDirectory))
            {
                Directory.Delete(workingDirectory, true);
            }

            Directory.CreateDirectory(workingDirectory);
            this.workingDirectory = workingDirectory;
            this.installerHashes = new ();
        }

        public void AddInstaller(string installer, string token)
        {
            this.installerHashes.Add(installer, token);
            this.PrepareInstaller(installer);
        }

        public void AddMsixInstaller(string installer, string token, string signatureToken)
        {
            this.installerHashes.AddMsix(installer, token, signatureToken);
            this.PrepareInstaller(installer);
        }

        public void CreateSource(string indexName, string inputManifests)
        {
            var indexCreator = new WinGetIndexCreator(this.workingDirectory);

            indexCreator.PrepareManifests(inputManifests, this.installerHashes.InstallerTokens);
            indexCreator.CreateIndex(indexName);

            // TODO: create msix.
        }

        private void PrepareInstaller(string installer)
        {
            // TODO: sign.
            File.Copy(installer, this.workingDirectory, true);
        }
    }
}
