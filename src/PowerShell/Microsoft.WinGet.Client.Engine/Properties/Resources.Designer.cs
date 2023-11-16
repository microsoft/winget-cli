﻿//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//     Runtime Version:4.0.30319.42000
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
// </auto-generated>
//------------------------------------------------------------------------------

namespace Microsoft.WinGet.Resources {
    using System;
    
    
    /// <summary>
    ///   A strongly-typed resource class, for looking up localized strings, etc.
    /// </summary>
    // This class was auto-generated by the StronglyTypedResourceBuilder
    // class via a tool like ResGen or Visual Studio.
    // To add or remove a member, edit your .ResX file then rerun ResGen
    // with the /str option, or rebuild your VS project.
    [global::System.CodeDom.Compiler.GeneratedCodeAttribute("System.Resources.Tools.StronglyTypedResourceBuilder", "17.0.0.0")]
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
    [global::System.Runtime.CompilerServices.CompilerGeneratedAttribute()]
    internal class Resources {
        
        private static global::System.Resources.ResourceManager resourceMan;
        
        private static global::System.Globalization.CultureInfo resourceCulture;
        
        [global::System.Diagnostics.CodeAnalysis.SuppressMessageAttribute("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode")]
        internal Resources() {
        }
        
        /// <summary>
        ///   Returns the cached ResourceManager instance used by this class.
        /// </summary>
        [global::System.ComponentModel.EditorBrowsableAttribute(global::System.ComponentModel.EditorBrowsableState.Advanced)]
        internal static global::System.Resources.ResourceManager ResourceManager {
            get {
                if (object.ReferenceEquals(resourceMan, null)) {
                    global::System.Resources.ResourceManager temp = new global::System.Resources.ResourceManager("Microsoft.WinGet.Client.Engine.Properties.Resources", typeof(Resources).Assembly);
                    resourceMan = temp;
                }
                return resourceMan;
            }
        }
        
        /// <summary>
        ///   Overrides the current thread's CurrentUICulture property for all
        ///   resource lookups using this strongly typed resource class.
        /// </summary>
        [global::System.ComponentModel.EditorBrowsableAttribute(global::System.ComponentModel.EditorBrowsableState.Advanced)]
        internal static global::System.Globalization.CultureInfo Culture {
            get {
                return resourceCulture;
            }
            set {
                resourceCulture = value;
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to An error occurred while connecting to the catalog..
        /// </summary>
        internal static string CatalogConnectExceptionMessage {
            get {
                return ResourceManager.GetString("CatalogConnectExceptionMessage", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Debug parameter not supported.
        /// </summary>
        internal static string DebugNotSupported {
            get {
                return ResourceManager.GetString("DebugNotSupported", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to An error occurred while searching for packages: {0}.
        /// </summary>
        internal static string FindPackagesExceptionMessage {
            get {
                return ResourceManager.GetString("FindPackagesExceptionMessage", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to The App Execution Alias for the Windows Package Manager is disabled..
        /// </summary>
        internal static string IntegrityAppExecutionAliasDisabledMessage {
            get {
                return ResourceManager.GetString("IntegrityAppExecutionAliasDisabledMessage", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to No applicable license found..
        /// </summary>
        internal static string IntegrityAppInstallerLicense {
            get {
                return ResourceManager.GetString("IntegrityAppInstallerLicense", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to The App Installer is not installed..
        /// </summary>
        internal static string IntegrityAppInstallerNotInstalledMessage {
            get {
                return ResourceManager.GetString("IntegrityAppInstallerNotInstalledMessage", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to The App Installer is not registered..
        /// </summary>
        internal static string IntegrityAppInstallerNotRegisteredMessage {
            get {
                return ResourceManager.GetString("IntegrityAppInstallerNotRegisteredMessage", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to The App Installer does not contain the Windows Package Manager..
        /// </summary>
        internal static string IntegrityAppInstallerNotSupportedMessage {
            get {
                return ResourceManager.GetString("IntegrityAppInstallerNotSupportedMessage", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Windows Package Manager returned an unexcepted result..
        /// </summary>
        internal static string IntegrityFailureMessage {
            get {
                return ResourceManager.GetString("IntegrityFailureMessage", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to The App Installer did not automatically add the PATH environment variable..
        /// </summary>
        internal static string IntegrityNotInPathMessage {
            get {
                return ResourceManager.GetString("IntegrityNotInPathMessage", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to The Windows Package Manager requires Windows Version 1809 (October 2018 Update) or later..
        /// </summary>
        internal static string IntegrityOsNotSupportedMessage {
            get {
                return ResourceManager.GetString("IntegrityOsNotSupportedMessage", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to The installed winget version doesn&apos;t match the expectation. Installer version &apos;{0}&apos; Expected version &apos;{1}&apos;.
        /// </summary>
        internal static string IntegrityUnexpectedVersionMessage {
            get {
                return ResourceManager.GetString("IntegrityUnexpectedVersionMessage", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Unable to execute winget command..
        /// </summary>
        internal static string IntegrityUnknownMessage {
            get {
                return ResourceManager.GetString("IntegrityUnknownMessage", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to No source matches the given value: {0}.
        /// </summary>
        internal static string InvalidSourceExceptionMessage {
            get {
                return ResourceManager.GetString("InvalidSourceExceptionMessage", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to No versions matched the given value: {0}.
        /// </summary>
        internal static string InvalidVersionExceptionMessage {
            get {
                return ResourceManager.GetString("InvalidVersionExceptionMessage", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Microsoft.UI.Xaml.2.7 package is not installed.
        /// </summary>
        internal static string MicrosoftUIXaml27Message {
            get {
                return ResourceManager.GetString("MicrosoftUIXaml27Message", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to No packages matched the given input criteria..
        /// </summary>
        internal static string NoPackageFoundExceptionMessage {
            get {
                return ResourceManager.GetString("NoPackageFoundExceptionMessage", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Installing &apos;{0}&apos;.
        /// </summary>
        internal static string ProgressRecordActivityInstalling {
            get {
                return ResourceManager.GetString("ProgressRecordActivityInstalling", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Uninstalling &apos;{0}&apos;.
        /// </summary>
        internal static string ProgressRecordActivityUninstalling {
            get {
                return ResourceManager.GetString("ProgressRecordActivityUninstalling", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Updating &apos;{0}&apos;.
        /// </summary>
        internal static string ProgressRecordActivityUpdating {
            get {
                return ResourceManager.GetString("ProgressRecordActivityUpdating", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Try running with -AllUsers in administrator mode..
        /// </summary>
        internal static string RepairAllUsersHelpMessage {
            get {
                return ResourceManager.GetString("RepairAllUsersHelpMessage", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to -AllUsers requires administrator mode..
        /// </summary>
        internal static string RepairAllUsersMessage {
            get {
                return ResourceManager.GetString("RepairAllUsersMessage", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to The App Execution Alias for the Windows Package Manager is disabled. You should enable the App Execution Alias for the Windows Package Manager. Go to App execution aliases option in Apps &amp; features Settings to enable it..
        /// </summary>
        internal static string RepairAppExecutionAliasMessage {
            get {
                return ResourceManager.GetString("RepairAppExecutionAliasMessage", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Failed to repair winget..
        /// </summary>
        internal static string RepairFailureMessage {
            get {
                return ResourceManager.GetString("RepairFailureMessage", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to This cmdlet requires administrator privileges to execute..
        /// </summary>
        internal static string RequiresAdminMessage {
            get {
                return ResourceManager.GetString("RequiresAdminMessage", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Single threaded apartment (STA) is not currently supported in this context; run PowerShell in Multi-threaded apartment mode (MTA)..
        /// </summary>
        internal static string SingleThreadedApartmentNotSupportedMessage {
            get {
                return ResourceManager.GetString("SingleThreadedApartmentNotSupportedMessage", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to User settings file is invalid..
        /// </summary>
        internal static string UserSettingsReadException {
            get {
                return ResourceManager.GetString("UserSettingsReadException", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to {0}, {1}, and {2} other packages matched the input criteria. Please refine the input..
        /// </summary>
        internal static string VagueCriteriaExceptionMessage {
            get {
                return ResourceManager.GetString("VagueCriteriaExceptionMessage", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to This cmdlet is not supported in Windows PowerShell..
        /// </summary>
        internal static string WindowsPowerShellNotSupported {
            get {
                return ResourceManager.GetString("WindowsPowerShellNotSupported", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Winget command &apos;{0}&apos; with parameters &apos;{1}&apos; failed with exit code &apos;{2}&apos;..
        /// </summary>
        internal static string WinGetCLIExceptionMessage {
            get {
                return ResourceManager.GetString("WinGetCLIExceptionMessage", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Winget command timed out: {0} {1}.
        /// </summary>
        internal static string WinGetCLITimeoutExceptionMessage {
            get {
                return ResourceManager.GetString("WinGetCLITimeoutExceptionMessage", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Windows Package Manager not supported..
        /// </summary>
        internal static string WinGetNotSupportedMessage {
            get {
                return ResourceManager.GetString("WinGetNotSupportedMessage", resourceCulture);
            }
        }
    }
}
