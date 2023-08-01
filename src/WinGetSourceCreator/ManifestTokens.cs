// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace Microsoft.WinGetSourceCreator
{
    using System.Security.Cryptography;

    public class ManifestTokens
    {
        public ManifestTokens()
        {
        }

        public Dictionary<string, string> Tokens { get; private set; } = new Dictionary<string, string>();

        public void AddHashToken(string file, string token)
        {
            if (!token.StartsWith("<") || !token.EndsWith(">"))
            {
                throw new Exception("Token should be in the form of <TOKEN VALUE>");
            }

            var hash = HashFile(file);
            this.Tokens.Add(token, hash);
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
    }
}
