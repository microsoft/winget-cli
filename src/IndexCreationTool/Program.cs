// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace IndexCreationTool
{
    using Microsoft.WinGetSourceCreator;
    using System;
    using System.IO;
    using System.Text.Json;
    using System.Text.Json.Serialization;
    using WinGetSourceCreator.Model;

    class Program
    {
        private static void PrintUsage()
        {
            Console.WriteLine("Usage: IndexCreationTool.exe -f <local source input json file>");
        }

        private static LocalSource ParseArguments(string[] args)
        {
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

            var content = File.ReadAllText(inputFile);
            content = Environment.ExpandEnvironmentVariables(content);

            var options = new JsonSerializerOptions
            {
                PropertyNameCaseInsensitive = true,
                Converters =
                {
                    new JsonStringEnumConverter(JsonNamingPolicy.CamelCase)
                }
            };

            return JsonSerializer.Deserialize<LocalSource>(content, options);
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
