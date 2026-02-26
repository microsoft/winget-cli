// -----------------------------------------------------------------------------
// <copyright file="PathEnvironmentVariableHandler.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Helpers
{
    using System;
    using System.Collections.Generic;

    /// <summary>
    /// Class for handling PATH environment variable.
    /// </summary>
    public static class PathEnvironmentVariableHandler
    {
        private const string PathEnvironmentVariable = "PATH";

        private static readonly object EnvironmentVariableLock = new object();

        /// <summary>
        /// Gets the lock to read or write PATH environment variable.
        /// </summary>
        public static object Lock
        {
            get { return EnvironmentVariableLock; }
        }

        /// <summary>
        /// Updates the process's PATH environment variable if new paths added.
        /// Only adds new paths since we add to PATH in other code which may not be in the registry.
        /// </summary>
        public static void UpdatePath()
        {
            HashSet<string> paths = new HashSet<string>(Environment.GetEnvironmentVariable(PathEnvironmentVariable)?.Split(';') ?? Array.Empty<string>());
            var originalPathsSize = paths.Count;

            AddPathsIfNotExist(paths, Environment.GetEnvironmentVariable(PathEnvironmentVariable, EnvironmentVariableTarget.Machine)?.Split(';'));
            AddPathsIfNotExist(paths, Environment.GetEnvironmentVariable(PathEnvironmentVariable, EnvironmentVariableTarget.User)?.Split(';'));

            if (paths.Count > originalPathsSize)
            {
                lock (Lock)
                {
                    Environment.SetEnvironmentVariable(PathEnvironmentVariable, string.Join(';', paths));
                }
            }
        }

        // TODO: Currently it always adds new paths to the end. The "proper" thing to do would probably be to calculate
        // the full new list of paths (what one would expect to get from a new process launch) and use a line merge algorithm
        // with a strategy that puts the ephemeral entries before the new permanent ones.
#pragma warning disable SA1011 // Closing square brackets should be spaced correctly
        private static void AddPathsIfNotExist(HashSet<string> currentPaths, string[]? paths)
#pragma warning restore SA1011 // Closing square brackets should be spaced correctly
        {
            if (paths is not null)
            {
                foreach (var path in paths)
                {
                    if (!currentPaths.Contains(path))
                    {
                        currentPaths.Add(path);
                    }
                }
            }
        }
    }
}
