// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace Microsoft.WinGetSourceCreator
{
    using System.Security.Cryptography;

    public class WinGetInstallerHashes
    {
        public WinGetInstallerHashes()
        {
        }

        public Dictionary<string, string> InstallerTokens { get; private set; } = new Dictionary<string, string>();

        public void Add(string installer, string token)
        {
            if (!token.StartsWith("<") || !token.EndsWith(">"))
            {
                throw new Exception("Token should be in the form of <TOKEN VALUE>");
            }

            var hash = HashFile(installer);
            this.InstallerTokens.Add(token, hash);
        }

        public void AddMsix(string installer, string token, string signatureToken)
        {
            this.Add(installer, token);

            var signatureFilePath = GetSignatureFileFromMsix(installer);
            this.Add(signatureFilePath, signatureToken);

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

        /// <summary>
        /// Gets the hash of the specified file.
        /// </summary>
        /// <param name="filePath">File path.</param>
        /// <returns>Hash of file.</returns>
        private static string HashFile(string filePath)
        {
            if (!File.Exists(filePath))
            {
                throw new FileNotFoundException(filePath);
            }

            string hash = string.Empty;

            using SHA256 mySHA256 = SHA256.Create();
            using FileStream fs = File.OpenRead(filePath);
            fs.Position = 0;
            byte[] hashValue = mySHA256.ComputeHash(fs);

            for (int i = 0; i < hashValue.Length; i++)
            {
                hash += $"{hashValue[i]:X2}";
            }

            return hash;
        }

        /// <summary>
        /// Gets hash of the AppxSignature.p7x file in the msix.
        /// </summary>
        /// <param name="packageFilePath">Package file path.</param>
        /// <returns>Hash of signature file.</returns>
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
    }
}
