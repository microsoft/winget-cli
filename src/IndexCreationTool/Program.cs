// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace IndexCreationTool
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using WinGetUtilInterop.Helpers;

    class Program
    {
        public const string IndexName = @"index.db";
        public const string IndexPathInPackage = @"Public\index.db";
        public const string IndexPackageName = @"source.msix";

        static int Main(string[] args)
        {
            string rootDir = string.Empty;
            string appxManifestPath = string.Empty;
            string certPath = string.Empty;

            // List of directories to include. By default, include all directories.
            List<string> includeDirList = new() { string.Empty };

            for (int i = 0; i < args.Length; i++)
            {
                if (args[i] == "-d" && ++i < args.Length)
                {
                    rootDir = args[i];
                }
                else if (args[i] == "-i" && ++i < args.Length)
                {
                    includeDirList = args[i].Split(",").ToList();
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
                Console.WriteLine("Usage: IndexCreationTool.exe -d <Path to search for yaml> [-i <Comma-separated list of included dirs in the search for yaml. If not specified, include all dirs>] [-m <appxmanifest for index package> [-c <cert for signing index package>]]");
                return 0;
            }

            try
            {
                if (File.Exists(IndexName))
                {
                    File.Delete(IndexName);
                }

                using var indexHelper = WinGetUtilIndex.CreateLatestVersion(IndexName);
                foreach (string includeDir in includeDirList)
                {
                    var fullPath = Path.Combine(rootDir, includeDir);
                    foreach (string file in Directory.EnumerateFiles(fullPath, "*.yaml", SearchOption.AllDirectories))
                    {
<<<<<<< HEAD
                        var fullPath = Path.Combine(rootDir, includeDir);
                        Queue<string> filesQueue = new(Directory.EnumerateFiles(fullPath, "*.yaml", SearchOption.AllDirectories));

                        while (filesQueue.Count > 0)
                        {
                            int currentCount = filesQueue.Count;

                            for (int i = 0; i < currentCount; i++)
                            {
                                string file = filesQueue.Dequeue();
                                try
                                {
                                    indexHelper.AddManifest(file, Path.GetRelativePath(rootDir, file));
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
                                Console.WriteLine("Failed to add all manifests in directory to index.");
                                Environment.Exit(-1);
                            }
                        }
                    }

                    indexHelper.PrepareForPackaging();
=======
                        indexHelper.AddManifest(file, Path.GetRelativePath(rootDir, file));
                    }
>>>>>>> 8a4c811c (Start tool)
                }
                indexHelper.PrepareForPackaging();

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
                return -1;
            }

            return 0;
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