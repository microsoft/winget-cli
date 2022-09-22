using System;
using System.Runtime.InteropServices;

namespace AppInstallerCLIE2ETests.WinGetUtil
{
    internal class WinGetUtilWrapper
    {
        private const string DllName = @"WinGetUtil.dll";

        [Flags]
        public enum CreateManifestOption
        {
            NoValidation = 0,
        }

        [Flags]
        public enum ValidateManifestResultCode
        {
            Success = 0,
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
        }


        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern IntPtr WinGetCompareVersions(string version1, string version2, [MarshalAs(UnmanagedType.U4)] out int comparisonResult);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern IntPtr WinGetDownload(string url, string filePath, [MarshalAs(UnmanagedType.LPArray)] byte[] sha26Hash, uint sha256HashLength);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern IntPtr WinGetLoggingInit(string logPath);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern IntPtr WinGetLoggingTerm(string logPath);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern IntPtr WinGetCreateManifest(
            string inputPath,
            [MarshalAs(UnmanagedType.U1)] out bool succeeded,
            out IntPtr manifestHandle,
            [MarshalAs(UnmanagedType.BStr)] out string failureMessage,
            string mergedManifestPath,
            CreateManifestOption option);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern IntPtr WinGetCloseManifest(IntPtr manifest);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern IntPtr WinGetValidateManifestV3(
            IntPtr manifestHandle,
            IntPtr indexHandle,
            out ValidateManifestResultCode result,
            [MarshalAs(UnmanagedType.BStr)] out string failureMessage,
            ValidateManifestOptionV2 option,
            ValidateManifestOperationType operationType);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern IntPtr WinGetSQLiteIndexCreate(string filePath, uint majorVersion, uint minorVersion, out IntPtr index);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern IntPtr WinGetSQLiteIndexOpen(string filePath, out IntPtr index);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern IntPtr WinGetSQLiteIndexClose(IntPtr index);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern IntPtr WinGetSQLiteIndexAddManifest(IntPtr index, string manifestPath, string relativePath);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern IntPtr WinGetSQLiteIndexUpdateManifest(
            IntPtr index,
            string manifestPath,
            string relativePath,
            [MarshalAs(UnmanagedType.U1)] out bool indexModified);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern IntPtr WinGetSQLiteIndexRemoveManifest(IntPtr index, string manifestPath, string relativePath);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern IntPtr WinGetSQLiteIndexPrepareForPackaging(IntPtr index);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode, PreserveSig = false)]
        public static extern IntPtr WinGetSQLiteIndexCheckConsistency(IntPtr index, [MarshalAs(UnmanagedType.U1)] out bool succeeded);
    }
}
