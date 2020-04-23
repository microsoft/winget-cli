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

        static void Main(string[] args)
        {
            string rootDir = string.Empty;
            string appxManifestPath = string.Empty;
            string certPath = string.Empty;

            for (int i = 0; i < args.Length; i++)
            {
                if (args[i] == "-d" && ++i < args.Length)
                {
                    rootDir = args[i];
                }
                else if (args[i] == "-m" && ++i < args.Length)
                {
                    appxManifestPath = args[i];
                }
                else if (args[i] == "-c" && ++i < args.Length)
                {
                    certPath = args[i];
                }
            }

            if (string.IsNullOrEmpty(rootDir))
            {
                Console.WriteLine("Usage: IndexCreationTool.exe -d <Path to search for yaml> [-m <appxmanifest for index package> [-c <cert for signing index package>]]");
                return;
            }

            try
            {
                if (File.Exists(IndexName))
                {
                    File.Delete(IndexName);
                }

                using (var indexHelper = AppInstallerSQLiteIndexUtilWrapper.Create(IndexName))
                {
                    foreach (string file in Directory.EnumerateFiles(rootDir, "*.yaml", SearchOption.AllDirectories))
                    {
                        indexHelper.AddManifest(file, Path.GetRelativePath(rootDir, file));
                    }
                    indexHelper.PrepareForPackaging();
                }

                if (!string.IsNullOrEmpty(appxManifestPath))
                {
                    using (StreamWriter outputFile = new StreamWriter("MappingFile.txt"))
                    {
                        outputFile.WriteLine("[Files]");
                        outputFile.WriteLine($"\"{IndexName}\" \"{IndexPathInPackage}\"");
                        outputFile.WriteLine($"\"{appxManifestPath}\" \"AppxManifest.xml\"");
                    }

                    RunCommand("makeappx.exe", $"pack /f MappingFile.txt /o /nv /p {IndexPackageName}");

                    if (!string.IsNullOrEmpty(certPath))
                    {
                        RunCommand("signtool.exe", $"sign /a /fd sha256 /f {certPath} {IndexPackageName}");
                    }
                }
            }
            catch (Exception e)
            {
                Console.WriteLine("Failed. Reason: " + e.Message);
            }

            Environment.Exit(0);
        }

        static void RunCommand(string command, string args)
        {
            Process p = new Process();
            p.StartInfo = new ProcessStartInfo(command, args);
            p.Start();
            p.WaitForExit();
        }
    }
}
