// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

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

        [Flags]
        public enum CreateManifestOption
        {
            NoValidation = 0,
            SchemaValidation = 0x1,
            SchemaAndSemanticValidation = 0x2,
            ReturnErrorOnVerifiedPublisherFields = 0x1000,
        }

        [Flags]
        public enum ValidateManifestResultCode
        {
            Success = 0,
            DependenciesValidationFailure = 0x1,
            ArpVersionValidationFailure = 0x2,
            InstallerValidationFailure = 0x4,
            SingleManifestPackageHasDependencies = 0x10000,
            MultiManifestPackageHasDependencies = 0x20000,
            MissingManifestDependenciesNode = 0x40000,
            NoSuitableMinVersionDependency = 0x80000,
            FoundDependencyLoop = 0x100000,
            InternalError = 0x1000,
        }

        [Flags]
        public enum ValidateManifestOptionV2
        {
            None = 0,
            DependenciesValidation = 0x1,
            ArpVersionValidation = 0x2,
            InstallerValidation = 0x4,
        }

        public enum ValidateManifestOperationType
        {
            Add = 0,
            Update = 1,
            Delete = 2,
        }

        public enum WinGetBeginInstallerMetadataCollectionOptions
        {
            WinGetBeginInstallerMetadataCollectionOption_None = 0,
            WinGetBeginInstallerMetadataCollectionOption_InputIsFilePath = 0x1,
            WinGetBeginInstallerMetadataCollectionOption_InputIsURI = 0x2,
        };

        public enum WinGetCompleteInstallerMetadataCollectionOptions
        {
            WinGetCompleteInstallerMetadataCollectionOption_None = 0,
            WinGetCompleteInstallerMetadataCollectionOption_Abandon = 0x1,
        };

        public enum WinGetMergeInstallerMetadataOptions
        {
            WinGetMergeInstallerMetadataOptions_None = 0,
        };

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern void WinGetCompareVersions(string version1, string version2, [MarshalAs(UnmanagedType.U4)] out int comparisonResult);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern void WinGetDownload(string url, string filePath, [MarshalAs(UnmanagedType.LPArray)] byte[] sha26Hash, uint sha256HashLength);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern void WinGetLoggingInit(string logPath);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern void WinGetLoggingTerm(string logPath);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern void WinGetCreateManifest(
            string inputPath,
            [MarshalAs(UnmanagedType.U1)] out bool succeeded,
            out IntPtr manifestHandle,
            [MarshalAs(UnmanagedType.BStr)] out string failureMessage,
            string mergedManifestPath,
            CreateManifestOption option);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern void WinGetCloseManifest(IntPtr manifest);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern void WinGetValidateManifestV3(
            IntPtr manifestHandle,
            IntPtr indexHandle,
            out ValidateManifestResultCode result,
            [MarshalAs(UnmanagedType.BStr)] out string failureMessage,
            ValidateManifestOptionV2 option,
            ValidateManifestOperationType operationType);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern void WinGetSQLiteIndexCreate(string filePath, uint majorVersion, uint minorVersion, out IntPtr index);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern void WinGetSQLiteIndexOpen(string filePath, out IntPtr index);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern void WinGetSQLiteIndexClose(IntPtr index);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern void WinGetSQLiteIndexAddManifest(IntPtr index, string manifestPath, string relativePath);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern void WinGetSQLiteIndexUpdateManifest(
            IntPtr index,
            string manifestPath,
            string relativePath,
            [MarshalAs(UnmanagedType.U1)] out bool indexModified);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern void WinGetSQLiteIndexRemoveManifest(IntPtr index, string manifestPath, string relativePath);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern void WinGetSQLiteIndexPrepareForPackaging(IntPtr index);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern void WinGetSQLiteIndexCheckConsistency(IntPtr index, [MarshalAs(UnmanagedType.U1)] out bool succeeded);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern void WinGetBeginInstallerMetadataCollection(
            string inputJSON,
            string logFilePath,
            WinGetBeginInstallerMetadataCollectionOptions options,
            out IntPtr collectionHandle);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern void WinGetCompleteInstallerMetadataCollection(
            IntPtr collectionHandle,
            string outputFilePath,
            WinGetCompleteInstallerMetadataCollectionOptions options);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern void WinGetMergeInstallerMetadata(
            string inputJSON,
            [MarshalAs(UnmanagedType.BStr)] out string outputJSON,
            uint maximumOutputSizeInBytes,
            string logFilePath,
            WinGetMergeInstallerMetadataOptions options);
    }
}
