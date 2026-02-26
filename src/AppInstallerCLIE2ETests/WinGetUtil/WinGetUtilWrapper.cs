// -----------------------------------------------------------------------------
// <copyright file="WinGetUtilWrapper.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests.WinGetUtil
{
    using System;
    using System.Runtime.InteropServices;

    /// <summary>
    /// Wrapper class for WinGetUtil exports.
    /// For more details about methods in this class visit WinGetUtil.h file.
    /// </summary>
    public class WinGetUtilWrapper
    {
        private const string DllName = @"WinGetUtil.dll";

        /// <summary>
        /// Create manifest flags.
        /// </summary>
        [Flags]
        public enum CreateManifestOption
        {
            /// <summary>
            /// No validation.
            /// </summary>
            NoValidation = 0,

            /// <summary>
            /// Schema validation.
            /// </summary>
            SchemaValidation = 0x1,

            /// <summary>
            /// Schema and semantic validation.
            /// </summary>
            SchemaAndSemanticValidation = 0x2,

            /// <summary>
            /// Return error on verified publisher.
            /// </summary>
            ReturnErrorOnVerifiedPublisherFields = 0x1000,
        }

        /// <summary>
        /// Validate manifests results.
        /// </summary>
        [Flags]
        public enum ValidateManifestResultCode
        {
            /// <summary>
            /// Success.
            /// </summary>
            Success = 0,

            /// <summary>
            /// Dependencies validation failure.
            /// </summary>
            DependenciesValidationFailure = 0x1,

            /// <summary>
            /// Arp version validation failure.
            /// </summary>
            ArpVersionValidationFailure = 0x2,

            /// <summary>
            /// Installer validation failure.
            /// </summary>
            InstallerValidationFailure = 0x4,

            /// <summary>
            /// Single manifest package has dependencies.
            /// </summary>
            SingleManifestPackageHasDependencies = 0x10000,

            /// <summary>
            /// Multi manifest package has dependencies.
            /// </summary>
            MultiManifestPackageHasDependencies = 0x20000,

            /// <summary>
            /// Missing manifest dependencies.
            /// </summary>
            MissingManifestDependenciesNode = 0x40000,

            /// <summary>
            /// No suitable min version dependencies.
            /// </summary>
            NoSuitableMinVersionDependency = 0x80000,

            /// <summary>
            /// Found dependency loop.
            /// </summary>
            FoundDependencyLoop = 0x100000,

            /// <summary>
            /// Internal error.
            /// </summary>
            InternalError = 0x1000,
        }

        /// <summary>
        /// ValidateManifestOptionV2 flags.
        /// </summary>
        [Flags]
        public enum ValidateManifestOptionV2
        {
            /// <summary>
            /// None.
            /// </summary>
            None = 0,

            /// <summary>
            /// Dependencies validation.
            /// </summary>
            DependenciesValidation = 0x1,

            /// <summary>
            /// Arp version validation.
            /// </summary>
            ArpVersionValidation = 0x2,

            /// <summary>
            /// Installer validation.
            /// </summary>
            InstallerValidation = 0x4,
        }

        /// <summary>
        /// Validate manifest operation type.
        /// </summary>
        public enum ValidateManifestOperationType
        {
            /// <summary>
            /// Add.
            /// </summary>
            Add = 0,

            /// <summary>
            /// Update.
            /// </summary>
            Update = 1,

            /// <summary>
            /// Delete.
            /// </summary>
            Delete = 2,
        }

        /// <summary>
        /// Begin installer metadata collection options.
        /// </summary>
        public enum WinGetBeginInstallerMetadataCollectionOptions
        {
            /// <summary>
            /// None.
            /// </summary>
            WinGetBeginInstallerMetadataCollectionOption_None = 0,

            /// <summary>
            /// Input is file path.
            /// </summary>
            WinGetBeginInstallerMetadataCollectionOption_InputIsFilePath = 0x1,

            /// <summary>
            /// Input is URI.
            /// </summary>
            WinGetBeginInstallerMetadataCollectionOption_InputIsURI = 0x2,
        }

        /// <summary>
        /// Complete installer metadata collection.
        /// </summary>
        public enum WinGetCompleteInstallerMetadataCollectionOptions
        {
            /// <summary>
            /// None.
            /// </summary>
            WinGetCompleteInstallerMetadataCollectionOption_None = 0,

            /// <summary>
            /// Abandon.
            /// </summary>
            WinGetCompleteInstallerMetadataCollectionOption_Abandon = 0x1,
        }

        /// <summary>
        /// Merge installer metadata.
        /// </summary>
        public enum WinGetMergeInstallerMetadataOptions
        {
            /// <summary>
            /// None.
            /// </summary>
            WinGetMergeInstallerMetadataOptions_None = 0,
        }

        /// <summary>
        /// WinGetCompareVersion from wingetutil.dll .
        /// </summary>
        /// <param name="version1">Version.</param>
        /// <param name="version2">Other version.</param>
        /// <param name="comparisonResult">Result of comparison.</param>
        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern void WinGetCompareVersions(string version1, string version2, [MarshalAs(UnmanagedType.U4)] out int comparisonResult);

        /// <summary>
        /// WinGetDownload from wingetutil.dll .
        /// </summary>
        /// <param name="url">Url.</param>
        /// <param name="filePath">File path where to download.</param>
        /// <param name="sha256Hash">SHA256 hash.</param>
        /// <param name="sha256HashLength">SHA256 hash length.</param>
        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern void WinGetDownload(string url, string filePath, [MarshalAs(UnmanagedType.LPArray)] byte[] sha256Hash, uint sha256HashLength);

        /// <summary>
        /// WinGetLoggingInit from wingetutil.dll .
        /// </summary>
        /// <param name="logPath">Log path.</param>
        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern void WinGetLoggingInit(string logPath);

        /// <summary>
        /// WinGetLoggingTerm from wingetutil.dll .
        /// </summary>
        /// <param name="logPath">Log path.</param>
        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern void WinGetLoggingTerm(string logPath);

        /// <summary>
        /// WinGetCreateManifest from wingetutil.dll .
        /// </summary>
        /// <param name="inputPath">Input path.</param>
        /// <param name="succeeded">Succeeded.</param>
        /// <param name="manifestHandle">Manifest handle.</param>
        /// <param name="failureMessage">Failure message.</param>
        /// <param name="mergedManifestPath">Merge manifest path.</param>
        /// <param name="option">Option.</param>
        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern void WinGetCreateManifest(
            string inputPath,
            [MarshalAs(UnmanagedType.U1)] out bool succeeded,
            out IntPtr manifestHandle,
            [MarshalAs(UnmanagedType.BStr)] out string failureMessage,
            string mergedManifestPath,
            CreateManifestOption option);

        /// <summary>
        /// WinGetCloseManifest from wingetutil.dll .
        /// </summary>
        /// <param name="manifest">Manifest.</param>
        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern void WinGetCloseManifest(IntPtr manifest);

        /// <summary>
        /// WinGetValidateManifestV3 from wingetutil.dll .
        /// </summary>
        /// <param name="manifestHandle">Manifest handle.</param>
        /// <param name="indexHandle">Index handle.</param>
        /// <param name="result">Result.</param>
        /// <param name="failureMessage">Failure message.</param>
        /// <param name="option">Option.</param>
        /// <param name="operationType">Operation type.</param>
        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern void WinGetValidateManifestV3(
            IntPtr manifestHandle,
            IntPtr indexHandle,
            out ValidateManifestResultCode result,
            [MarshalAs(UnmanagedType.BStr)] out string failureMessage,
            ValidateManifestOptionV2 option,
            ValidateManifestOperationType operationType);

        /// <summary>
        /// WinGetSQLiteIndexCreate from wingetutil.dll .
        /// </summary>
        /// <param name="filePath">File path.</param>
        /// <param name="majorVersion">Major version.</param>
        /// <param name="minorVersion">Minor version.</param>
        /// <param name="index">Index.</param>
        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern void WinGetSQLiteIndexCreate(string filePath, uint majorVersion, uint minorVersion, out IntPtr index);

        /// <summary>
        /// WinGetSQLiteIndexOpen from wingetutil.dll .
        /// </summary>
        /// <param name="filePath">File path.</param>
        /// <param name="index">Index.</param>
        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern void WinGetSQLiteIndexOpen(string filePath, out IntPtr index);

        /// <summary>
        /// WinGetSQLiteIndexClose from wingetutil.dll .
        /// </summary>
        /// <param name="index">Index.</param>
        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern void WinGetSQLiteIndexClose(IntPtr index);

        /// <summary>
        /// WinGetSQLiteIndexAddManifest from wingetutil.dll .
        /// </summary>
        /// <param name="index">Index.</param>
        /// <param name="manifestPath">Manifest path.</param>
        /// <param name="relativePath">Relative path.</param>
        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern void WinGetSQLiteIndexAddManifest(IntPtr index, string manifestPath, string relativePath);

        /// <summary>
        /// WinGetSQLiteIndexUpdateManifest from wingetutil.dll .
        /// </summary>
        /// <param name="index">Index.</param>
        /// <param name="manifestPath">Manifest path.</param>
        /// <param name="relativePath">Relative path.</param>
        /// <param name="indexModified">Index modified.</param>
        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern void WinGetSQLiteIndexUpdateManifest(
            IntPtr index,
            string manifestPath,
            string relativePath,
            [MarshalAs(UnmanagedType.U1)] out bool indexModified);

        /// <summary>
        /// WinGetSQLiteIndexRemoveManifest from wingetutil.dll .
        /// </summary>
        /// <param name="index">Index.</param>
        /// <param name="manifestPath">Manifest path.</param>
        /// <param name="relativePath">Relative path.</param>
        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern void WinGetSQLiteIndexRemoveManifest(IntPtr index, string manifestPath, string relativePath);

        /// <summary>
        /// WinGetSQLiteIndexPrepareForPackaging from wingetutil.dll .
        /// </summary>
        /// <param name="index">Index.</param>
        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern void WinGetSQLiteIndexPrepareForPackaging(IntPtr index);

        /// <summary>
        /// WinGetSQLiteIndexCheckConsistency from wingetutil.dll .
        /// </summary>
        /// <param name="index">Index.</param>
        /// <param name="succeeded">Succeeded.</param>
        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern void WinGetSQLiteIndexCheckConsistency(IntPtr index, [MarshalAs(UnmanagedType.U1)] out bool succeeded);

        /// <summary>
        /// WinGetBeginInstallerMetadataCollection from wingetutil.dll .
        /// </summary>
        /// <param name="inputJSON">Input json.</param>
        /// <param name="logFilePath">Log file path.</param>
        /// <param name="options">Options.</param>
        /// <param name="collectionHandle">Collection handle.</param>
        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern void WinGetBeginInstallerMetadataCollection(
            string inputJSON,
            string logFilePath,
            WinGetBeginInstallerMetadataCollectionOptions options,
            out IntPtr collectionHandle);

        /// <summary>
        /// WinGetCompleteInstallerMetadataCollection from wingetutil.dll .
        /// </summary>
        /// <param name="collectionHandle">Collection handle.</param>
        /// <param name="outputFilePath">Output file path.</param>
        /// <param name="options">Options.</param>
        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern void WinGetCompleteInstallerMetadataCollection(
            IntPtr collectionHandle,
            string outputFilePath,
            WinGetCompleteInstallerMetadataCollectionOptions options);

        /// <summary>
        /// WinGetMergeInstallerMetadata from wingetutil.dll .
        /// </summary>
        /// <param name="inputJSON">Input json.</param>
        /// <param name="outputJSON">Output json.</param>
        /// <param name="maximumOutputSizeInBytes">Maximum output size in bytes.</param>
        /// <param name="logFilePath">Log file path.</param>
        /// <param name="options">Options.</param>
        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern void WinGetMergeInstallerMetadata(
            string inputJSON,
            [MarshalAs(UnmanagedType.BStr)] out string outputJSON,
            uint maximumOutputSizeInBytes,
            string logFilePath,
            WinGetMergeInstallerMetadataOptions options);
    }
}
