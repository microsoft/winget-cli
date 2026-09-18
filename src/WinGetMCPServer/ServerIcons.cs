// -----------------------------------------------------------------------------
// <copyright file="ServerIcons.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace WinGetMCPServer
{
    using System.Buffers.Binary;
    using ModelContextProtocol.Protocol;

    /// <summary>
    /// Provides the icons that describe this server and its tools.
    /// </summary>
    /// <remarks>
    /// <para>
    /// The unplated target size assets are selected deliberately. The only scaled asset the package
    /// carries for this logo is <c>AppList.scale-200.png</c>, which is an 88x88 image containing a
    /// 64x64 glyph; the surrounding transparent margin is intended for tile plating and makes the
    /// icon look small when a client renders it in a compact space. Every
    /// <c>AppList.targetsize-*_altform-unplated.png</c> asset is cropped to the glyph, so it fills the
    /// space it is given.
    /// </para>
    /// <para>
    /// Assets are located by file name rather than by a fixed relative path because the folder differs
    /// between the development package (<c>Images\</c>) and the shipped package
    /// (<c>Assets\WinGet\</c>); the file names are identical in both.
    /// </para>
    /// </remarks>
    internal static class ServerIcons
    {
        /// <summary>
        /// The icon sizes to look for, in pixels. These are sent on every <c>tools/list</c> response,
        /// so the set is kept small deliberately.
        /// </summary>
        private static readonly int[] RequestedSizes = [48, 96];

        /// <summary>
        /// Package relative directories that are known to hold the assets, tried before searching.
        /// </summary>
        private static readonly string[] CandidateDirectories = [@"Assets\WinGet", "Images", "Assets"];

        private static readonly Lazy<IList<Icon>?> LazyIcons = new(CreateIcons);

        /// <summary>
        /// Gets the icons describing this server, or <c>null</c> if they could not be read.
        /// </summary>
        public static IList<Icon>? Icons => LazyIcons.Value;

        private static IList<Icon>? CreateIcons()
        {
            try
            {
                string? packageRoot = GetPackageRoot();

                if (packageRoot == null)
                {
                    return null;
                }

                List<Icon> result = new List<Icon>();
                HashSet<string> alreadyAdded = new HashSet<string>();

                foreach (int size in RequestedSizes)
                {
                    string? iconFile = FindIconFile(packageRoot, size);

                    if (iconFile == null)
                    {
                        continue;
                    }

                    byte[] bytes = File.ReadAllBytes(iconFile);

                    if (bytes.Length == 0)
                    {
                        continue;
                    }

                    string source = "data:image/png;base64," + Convert.ToBase64String(bytes);

                    // Guard against two requested sizes resolving to the same asset; sending it more
                    // than once would only inflate every response carrying icons.
                    if (!alreadyAdded.Add(source))
                    {
                        continue;
                    }

                    result.Add(new Icon()
                    {
                        Source = source,
                        MimeType = "image/png",
                        Sizes = [DescribePngSize(bytes, size)],
                    });
                }

                return result.Count == 0 ? null : result;
            }
            catch (Exception)
            {
                // Icons are optional metadata. Any failure to read them must not prevent the server
                // from starting.
                return null;
            }
        }

        /// <summary>
        /// Gets the root of the containing package, which is the parent of the directory holding this
        /// executable.
        /// </summary>
        private static string? GetPackageRoot()
        {
            string baseDirectory = AppContext.BaseDirectory;

            if (string.IsNullOrEmpty(baseDirectory))
            {
                return null;
            }

            return Path.GetDirectoryName(baseDirectory.TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar));
        }

        private static string? FindIconFile(string packageRoot, int size)
        {
            // Preferring the unplated form keeps the glyph filling the icon. The plain target size
            // asset is currently identical to it, but is accepted as a fallback in case that changes.
            string[] fileNames =
            [
                $"AppList.targetsize-{size}_altform-unplated.png",
                $"AppList.targetsize-{size}.png",
            ];

            foreach (string fileName in fileNames)
            {
                foreach (string directory in CandidateDirectories)
                {
                    string path = Path.Combine(packageRoot, directory, fileName);

                    if (File.Exists(path))
                    {
                        return path;
                    }
                }
            }

            // The asset folder has been renamed before, so fall back to searching for the file rather
            // than failing outright.
            foreach (string fileName in fileNames)
            {
                try
                {
                    string? match = Directory.EnumerateFiles(packageRoot, fileName, SearchOption.AllDirectories).FirstOrDefault();

                    if (match != null)
                    {
                        return match;
                    }
                }
                catch (Exception)
                {
                    // Ignore directories that cannot be enumerated and continue with the next name.
                }
            }

            return null;
        }

        /// <summary>
        /// Determines the actual pixel dimensions of a PNG so that the advertised size matches the
        /// image being sent.
        /// </summary>
        private static string DescribePngSize(byte[] bytes, int requestedSize)
        {
            // A PNG begins with an 8 byte signature followed by the IHDR chunk. The chunk length
            // occupies the next 4 bytes and its type the 4 after that, placing the big endian width
            // and height at offsets 16 and 20.
            if (bytes.Length >= 24 &&
                bytes[12] == (byte)'I' && bytes[13] == (byte)'H' &&
                bytes[14] == (byte)'D' && bytes[15] == (byte)'R')
            {
                int width = BinaryPrimitives.ReadInt32BigEndian(bytes.AsSpan(16, 4));
                int height = BinaryPrimitives.ReadInt32BigEndian(bytes.AsSpan(20, 4));

                if (width > 0 && height > 0)
                {
                    return $"{width}x{height}";
                }
            }

            return $"{requestedSize}x{requestedSize}";
        }
    }
}
