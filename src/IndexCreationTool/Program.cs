// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace IndexCreationTool
{
    using System;
    using System.Diagnostics;
    using System.IO;

    class Program
    {
        public const string IndexName = @"index.db";
        public const string IndexPathInPackage = @"Public\index.db";
        public const string IndexPackageName = @"source.msix";
        public const string ManifestPathRootDir = @"manifests\m\microsoft";

        static void Main(string[] args)
        {
            // Add();
            // Update();
            // Verify();
            VerifyForDelete();
        }

        static void Add()
        {
            const string AddRootDirectory = @"C:\Users\akalagbe\OneDrive - Microsoft\Documents\dependencies-index-playground\add";
            if (File.Exists(IndexName))
            {
                File.Delete(IndexName);
            }

            string[] adds = new string[] { "Microsoft.MsGraphCLI", "Microsoft.EdgeBeta", "Microsoft.AzureCLI", "Microsoft.VisualStudioCode.Insiders" };
            using (var indexHelper = WinGetUtilWrapper.Create(IndexName))
            {
                foreach (var add in adds)
                {
                    string manifestRelativePath = Path.Combine(ManifestPathRootDir, add);
                    string filePath = Path.Combine(AddRootDirectory, $"{manifestRelativePath}.yaml");
                    indexHelper.AddManifest(filePath, manifestRelativePath);
                }
            }
        }

        static void Update()
        {
            const string UpdateRootDirectory = @"C:\Users\akalagbe\OneDrive - Microsoft\Documents\dependencies-index-playground\update";

            if (!File.Exists(IndexName))
            {
                throw new Exception($"File does not exists {IndexName}");
            }

            string[] adds = new string[] { "Microsoft.VisualStudioCode.Insiders" };
            using (var indexHelper = WinGetUtilWrapper.Open(IndexName))
            {
                foreach (var add in adds)
                {
                    string manifestRelativePath = Path.Combine(ManifestPathRootDir, add);
                    string filePath = Path.Combine(UpdateRootDirectory, $"{manifestRelativePath}.yaml");
                    indexHelper.UpdateManifest(filePath, manifestRelativePath);
                }
            }
        }

        static void Verify()
        {
            const string UpdateRootDirectory = @"C:\Users\akalagbe\OneDrive - Microsoft\Documents\dependencies-index-playground\verify";

            if (!File.Exists(IndexName))
            {
                throw new Exception($"File does not exists {IndexName}");
            }

            string[] adds = new string[] { "Microsoft.ADKPEAddon" };
            using (var indexHelper = WinGetUtilWrapper.Open(IndexName))
            {
                foreach (var add in adds)
                {
                    string manifestRelativePath = Path.Combine(ManifestPathRootDir, add);
                    string filePath = Path.Combine(UpdateRootDirectory, $"{manifestRelativePath}.yaml");
                    indexHelper.ValidateManifestV3(filePath, IndexName);
                }
            }
        }

        static void VerifyForDelete()
        {
            const string UpdateRootDirectory = @"C:\Users\akalagbe\OneDrive - Microsoft\Documents\dependencies-index-playground\verify";

            if (!File.Exists(IndexName))
            {
                throw new Exception($"File does not exists {IndexName}");
            }

            string[] adds = new string[] { "Microsoft.ADKPEAddon" };
            using (var indexHelper = WinGetUtilWrapper.Open(IndexName))
            {
                foreach (var add in adds)
                {
                    string manifestRelativePath = Path.Combine(ManifestPathRootDir, add);
                    string filePath = Path.Combine(UpdateRootDirectory, $"{manifestRelativePath}.yaml");
                    indexHelper.VerifyDependenciesStructureForManifestDelete(filePath, IndexName);
                }
            }
        }
    }
}