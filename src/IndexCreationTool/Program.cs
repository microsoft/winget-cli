﻿// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace IndexCreationTool
{
    using Microsoft.WinGetSourceCreator;
    using System;
    using System.IO;
    using System.Text.Json;

    class Program
    {
        private static void PrintUsage()
        {
            Console.WriteLine("Usage: IndexCreationTool.exe -f <local source input json file>");
        }

        private static LocalSource ParseArguments(string[] args)
        {
            // TODO: add other arguments and contruct json file.
            string inputFile = string.Empty;
            for (int i = 0; i < args.Length; i++)
            {
                if (args[i] == "-f" && ++i < args.Length)
                {
                    inputFile = args[i];
                }
            }

            if (string.IsNullOrEmpty(inputFile) || !File.Exists(inputFile))
            {
                PrintUsage();
                throw new ArgumentException("Missing input file");
            }

            var options = new JsonSerializerOptions
            {
                PropertyNameCaseInsensitive = true
            };
            return JsonSerializer.Deserialize<LocalSource>(File.ReadAllText(inputFile), options);
        }

        static int Main(string[] args)
        {
            try
            {
                var localSource = ParseArguments(args);
                WinGetLocalSource.CreateLocalSource(localSource);

            }
            catch (Exception e)
            {
                PrintUsage();
                Console.WriteLine(e.Message);
                return -1;
            }

            return 0;
        }

    }
}
