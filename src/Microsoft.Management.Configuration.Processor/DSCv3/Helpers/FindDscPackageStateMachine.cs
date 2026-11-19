// -----------------------------------------------------------------------------
// <copyright file="FindDscPackageStateMachine.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Helpers
{
    using System;
    using static Microsoft.Management.Configuration.Processor.DSCv3.Helpers.PackageInformation;

    /// <summary>
    /// Provides the state machine that decides which DSC package to use.
    /// </summary>
    internal class FindDscPackageStateMachine
    {
        private readonly Version minimumStableVersion = new Version(3, 1);
        private readonly Version minimumPreviewVersion = new Version(3, 1, 7);

        private State currentState = State.Initial;
        private string? dscExecutablePath;

        /// <summary>
        /// A state of the state machine.
        /// </summary>
        public enum State
        {
            /// <summary>
            /// The initial state.
            /// </summary>
            Initial,

            /// <summary>
            /// A stable installation attempt has been made.
            /// </summary>
            StableInstallAttempted,

            /// <summary>
            /// A preview installation attempt has been made.
            /// </summary>
            PreviewInstallAttempted,

            /// <summary>
            /// The state machine is terminated.
            /// </summary>
            Terminated,
        }

        /// <summary>
        /// A transition of the state machine.
        /// </summary>
        public enum Transition
        {
            /// <summary>
            /// Transition to a terminated state with DSC being found.
            /// </summary>
            Found,

            /// <summary>
            /// Attempt to install the stable version of DSC.
            /// </summary>
            InstallStable,

            /// <summary>
            /// Attempt to install the preview version of DSC.
            /// </summary>
            InstallPreview,

            /// <summary>
            /// Transition to a terminated state with DSC *not* being found.
            /// </summary>
            NotFound,
        }

        /// <summary>
        /// Gets or sets the diagnostics sink.
        /// </summary>
        public IDiagnosticsSink? DiagnosticsSink { get; set; }

        /// <summary>
        /// Gets the file path of the DSC (Desired State Configuration) executable.
        /// </summary>
        public string? DscExecutablePath
        {
            get
            {
                if (this.currentState == State.Terminated)
                {
                    return this.dscExecutablePath;
                }
                else
                {
                    PackageInformation stableInformation = new PackageInformation(StableDscPackageFamilyName);
                    if (stableInformation.IsInstalled && stableInformation.Version >= this.minimumStableVersion)
                    {
                        return stableInformation.GetExecutablePath(this.DiagnosticsSink);
                    }
                    else
                    {
                        PackageInformation previewInformation = new PackageInformation(PreviewDscPackageFamilyName);
                        if (previewInformation.IsInstalled && previewInformation.Version >= this.minimumPreviewVersion)
                        {
                            return previewInformation.GetExecutablePath(this.DiagnosticsSink);
                        }
                        else
                        {
                            return null;
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Determines the next state transition based on the current context or conditions.
        /// </summary>
        /// <returns>
        /// A string representing the name of the next transition.
        /// </returns>
        public Transition DetermineNextTransition()
        {
            switch (this.currentState)
            {
                case State.Initial:
                    {
                        PackageInformation stableInformation = new PackageInformation(StableDscPackageFamilyName);
                        if (stableInformation.IsInstalled && stableInformation.Version >= this.minimumStableVersion)
                        {
                            return this.Found(stableInformation);
                        }
                        else
                        {
                            this.currentState = State.StableInstallAttempted;
                            return Transition.InstallStable;
                        }
                    }

                case State.StableInstallAttempted:
                    {
                        PackageInformation stableInformation = new PackageInformation(StableDscPackageFamilyName);
                        if (stableInformation.IsInstalled && stableInformation.Version >= this.minimumStableVersion)
                        {
                            return this.Found(stableInformation);
                        }
                        else
                        {
                            PackageInformation previewInformation = new PackageInformation(PreviewDscPackageFamilyName);
                            if (previewInformation.IsInstalled && previewInformation.Version >= this.minimumPreviewVersion)
                            {
                                return this.Found(previewInformation);
                            }
                            else
                            {
                                this.currentState = State.PreviewInstallAttempted;
                                return Transition.InstallPreview;
                            }
                        }
                    }

                case State.PreviewInstallAttempted:
                    {
                        PackageInformation previewInformation = new PackageInformation(PreviewDscPackageFamilyName);
                        if (previewInformation.IsInstalled && previewInformation.Version >= this.minimumPreviewVersion)
                        {
                            return this.Found(previewInformation);
                        }
                        else
                        {
                            this.currentState = State.Terminated;
                            return Transition.NotFound;
                        }
                    }

                case State.Terminated:
                    return this.DscExecutablePath == null ? Transition.NotFound : Transition.Found;

                default:
                    throw new InvalidOperationException($"Unexpected state: {this.currentState}");
            }
        }

        private Transition Found(PackageInformation packageInformation)
        {
            this.dscExecutablePath = packageInformation.GetExecutablePath(this.DiagnosticsSink);
            this.currentState = State.Terminated;
            return Transition.Found;
        }
    }
}
