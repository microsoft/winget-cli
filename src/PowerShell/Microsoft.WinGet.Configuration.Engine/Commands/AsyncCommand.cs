// -----------------------------------------------------------------------------
// <copyright file="AsyncCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Engine.Commands
{
    using System;
    using System.Collections.Concurrent;
    using System.Management.Automation;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.PowerShell.Commands;
    using Microsoft.WinGet.Configuration.Engine.Exceptions;
    using Microsoft.WinGet.Configuration.Engine.Resources;
    using Microsoft.WinGet.SharedLib.Exceptions;
    using Microsoft.WinGet.SharedLib.PolicySettings;

    /// <summary>
    /// This is the base class for any command that performs async operations.
    /// It supports running tasks in an MTA thread via RunOnMta.
    /// If the thread is already running on an MTA it will executed it, otherwise
    /// it will create a new MTA thread.
    ///
    /// Wait must be used to synchronously wait con the task.
    /// </summary>
    public abstract class AsyncCommand
    {
        private static readonly string[] WriteInformationTags = new string[] { "PSHOST" };

        private readonly Thread originalThread;

        private readonly CancellationTokenSource source = new ();
        private BlockingCollection<QueuedStream> queuedStreams = new ();

        private int progressActivityId = 0;
        private ConcurrentDictionary<int, ProgressRecordType> progressRecords = new ();

        /// <summary>
        /// Initializes a new instance of the <see cref="AsyncCommand"/> class.
        /// </summary>
        /// <param name="psCmdlet">PSCmdlet.</param>
        public AsyncCommand(PSCmdlet psCmdlet)
        {
            // Passing Debug will make all the message actions to be Inquire. For async operations
            // and the current queue message implementation this doesn't make sense.
            // PowerShell will inquire for any message giving the impression that the task is
            // paused, but the async operation is still running.
            if (psCmdlet.MyInvocation.BoundParameters.ContainsKey("Debug"))
            {
                throw new NotSupportedException(Resources.DebugNotSupported);
            }

            GroupPolicy groupPolicy = GroupPolicy.GetInstance();

            if (!groupPolicy.IsEnabled(Policy.WinGet))
            {
                throw new GroupPolicyException(Policy.WinGet, GroupPolicyFailureType.BlockedByPolicy);
            }

            if (!groupPolicy.IsEnabled(Policy.Configuration))
            {
                throw new GroupPolicyException(Policy.Configuration, GroupPolicyFailureType.BlockedByPolicy);
            }

            if (!groupPolicy.IsEnabled(Policy.WinGetCommandLineInterfaces))
            {
                throw new GroupPolicyException(Policy.WinGetCommandLineInterfaces, GroupPolicyFailureType.BlockedByPolicy);
            }

            this.PsCmdlet = psCmdlet;
            this.originalThread = Thread.CurrentThread;
        }

        /// <summary>
        /// The write stream type.
        /// </summary>
        public enum StreamType
        {
            /// <summary>
            /// Debug.
            /// </summary>
            Debug,

            /// <summary>
            /// Verbose.
            /// </summary>
            Verbose,

            /// <summary>
            /// Warning.
            /// </summary>
            Warning,

            /// <summary>
            /// Error.
            /// </summary>
            Error,

            /// <summary>
            /// Progress.
            /// </summary>
            Progress,

            /// <summary>
            /// Object.
            /// </summary>
            Object,

            /// <summary>
            /// Information.
            /// </summary>
            Information,
        }

        /// <summary>
        /// Gets the base cmdlet.
        /// </summary>
        protected PSCmdlet PsCmdlet { get; private set; }

        /// <summary>
        /// Request cancellation for this command.
        /// </summary>
        public void Cancel()
        {
            this.source.Cancel();
        }

        /// <summary>
        /// Complete this operation.
        /// </summary>
        public virtual void Complete()
        {
            this.queuedStreams.CompleteAdding();
        }

        /// <summary>
        /// Execute the delegate in a MTA thread.
        /// </summary>
        /// <param name="func">Function to execute.</param>
        /// <returns>A <see cref="Task"/> representing the asynchronous operation.</returns>
        internal Task RunOnMTA(Func<Task> func)
        {
            // This must be called in the main thread.
            if (this.originalThread != Thread.CurrentThread)
            {
                throw new InvalidOperationException();
            }

            if (Thread.CurrentThread.GetApartmentState() == ApartmentState.MTA)
            {
                this.Write(StreamType.Verbose, "Already running on MTA");
                return func();
            }

            this.Write(StreamType.Verbose, "Creating MTA thread");
            var tcs = new TaskCompletionSource();
            var thread = new Thread(() =>
            {
                try
                {
                    func().GetAwaiter().GetResult();
                    tcs.SetResult();
                }
                catch (Exception e)
                {
                    tcs.SetException(e);
                }
            });

            thread.SetApartmentState(ApartmentState.MTA);
            thread.Start();
            return tcs.Task;
        }

        /// <summary>
        /// Execute the delegate in a MTA thread.
        /// </summary>
        /// <param name="func">Function to execute.</param>
        /// <typeparam name="TResult">Return type of function.</typeparam>
        /// <returns>A <see cref="Task"/> representing the asynchronous operation.</returns>
        internal Task<TResult> RunOnMTA<TResult>(Func<Task<TResult>> func)
        {
            // This must be called in the main thread.
            if (this.originalThread != Thread.CurrentThread)
            {
                throw new InvalidOperationException();
            }

            if (Thread.CurrentThread.GetApartmentState() == ApartmentState.MTA)
            {
                this.Write(StreamType.Verbose, "Already running on MTA");
                return func();
            }

            this.Write(StreamType.Verbose, "Creating MTA thread");
            var tcs = new TaskCompletionSource<TResult>();
            var thread = new Thread(() =>
            {
                try
                {
                    var result = func().GetAwaiter().GetResult();
                    tcs.SetResult(result);
                }
                catch (Exception e)
                {
                    tcs.SetException(e);
                }
            });

            thread.SetApartmentState(ApartmentState.MTA);
            thread.Start();
            return tcs.Task;
        }

        /// <summary>
        /// Waits for the task to be completed. This MUST be called from the main thread.
        /// </summary>
        /// <param name="runningTask">Task to wait for.</param>
        /// <param name="writeCommand">The command that can write to PowerShell.</param>
        internal void Wait(Task runningTask, AsyncCommand? writeCommand = null)
        {
            writeCommand ??= this;

            // This must be called in the main thread.
            if (this.originalThread != Thread.CurrentThread)
            {
                throw new InvalidOperationException();
            }

            do
            {
                this.ConsumeAndWriteStreams(writeCommand);
            }
            while (!(runningTask.IsCompleted && this.queuedStreams.IsCompleted));

            if (runningTask.IsFaulted)
            {
                // If IsFaulted is true, the task's Status is equal to Faulted,
                // and its Exception property will be non-null.
                throw runningTask.Exception!;
            }
        }

        /// <summary>
        /// Writes into the corresponding stream if running on the main thread.
        /// Otherwise queue the message.
        /// Is the caller responsibility to use the correct types.
        /// </summary>
        /// <param name="type">Stream type.</param>
        /// <param name="data">Data.</param>
        internal void Write(StreamType type, object data)
        {
            if (type == StreamType.Progress)
            {
                // Keep track of all progress activity.
                ProgressRecord progressRecord = (ProgressRecord)data;
                if (!this.progressRecords.TryAdd(progressRecord.ActivityId, progressRecord.RecordType))
                {
                    _ = this.progressRecords.TryUpdate(progressRecord.ActivityId, progressRecord.RecordType, ProgressRecordType.Completed);
                }
            }

            if (this.originalThread == Thread.CurrentThread)
            {
                this.CmdletWrite(type, data, this);
                return;
            }

            this.queuedStreams.Add(new QueuedStream(type, data));
        }

        /// <summary>
        /// Write error with an exception.
        /// </summary>
        /// <param name="errorId">Error id.</param>
        /// <param name="e">Exception.</param>
        internal void WriteError(ErrorRecordErrorId errorId, Exception e)
        {
            this.Write(
                StreamType.Error,
                new ErrorRecord(
                    e,
                    errorId.ToString(),
                    ErrorCategory.WriteError,
                    null));
        }

        /// <summary>
        /// Write error with a message. Create WriteErrorException.
        /// </summary>
        /// <param name="errorId">Error id.</param>
        /// <param name="message">Message.</param>
        /// <param name="e">Inner exception.</param>
        internal void WriteError(ErrorRecordErrorId errorId, string message, Exception? e = null)
        {
            // The error record requires a exception that can't be null, but there's no requirement that it was thrown.
            // If not specified use WriteErrorException.
            this.Write(
                StreamType.Error,
                new ErrorRecord(
                    new WriteErrorException(message, e),
                    errorId.ToString(),
                    ErrorCategory.WriteError,
                    null));
        }

        /// <summary>
        /// Helper to compute percentage and write progress for processing activities.
        /// </summary>
        /// <param name="activityId">Activity id.</param>
        /// <param name="activity">The activity in progress.</param>
        /// <param name="status">The status of the activity.</param>
        /// <param name="completed">Number of completed actions.</param>
        /// <param name="total">The expected total.</param>
        internal void WriteProgressWithPercentage(int activityId, string activity, string status, int completed, int total)
        {
            double percentComplete = (double)completed / total;
            var record = new ProgressRecord(activityId, activity, status)
            {
                RecordType = ProgressRecordType.Processing,
                PercentComplete = (int)(100.0 * percentComplete),
            };
            this.Write(StreamType.Progress, record);
        }

        /// <summary>
        /// Helper to complete progress records.
        /// </summary>
        /// <param name="activityId">Activity id.</param>
        /// <param name="activity">The activity in progress.</param>
        /// <param name="status">The status of the activity.</param>
        internal void CompleteProgress(int activityId, string activity, string status)
        {
            var record = new ProgressRecord(activityId, activity, status)
            {
                RecordType = ProgressRecordType.Completed,
                PercentComplete = 100,
            };
            this.Write(StreamType.Progress, record);
        }

        /// <summary>
        /// Writes to PowerShell streams.
        /// This method must be called in the original thread.
        /// WARNING: You must only call this when the task is completed or in Wait.
        /// </summary>
        /// <param name="writeCommand">The command that can write to PowerShell.</param>
        internal void ConsumeAndWriteStreams(AsyncCommand writeCommand)
        {
            // This must be called in the main thread.
            if (this.originalThread != Thread.CurrentThread)
            {
                throw new InvalidOperationException();
            }

            // Take from the blocking collection until is completed.
            try
            {
                while (true)
                {
                    var queuedOutput = this.queuedStreams.Take();
                    if (queuedOutput != null)
                    {
                        this.CmdletWrite(queuedOutput.Type, queuedOutput.Data, writeCommand);
                    }
                }
            }
            catch (InvalidOperationException)
            {
                // We are done.
                // An InvalidOperationException means that Take() was called on a completed collection.
            }
        }

        /// <summary>
        /// Gets a new progress activity id.
        /// </summary>
        /// <returns>The new progress record id.</returns>
        internal int GetNewProgressActivityId()
        {
            return Interlocked.Increment(ref this.progressActivityId);
        }

        /// <summary>
        /// Gets the cancellation token.
        /// </summary>
        /// <returns>CancellationToken.</returns>
        protected CancellationToken GetCancellationToken()
        {
            return this.source.Token;
        }

        private void CmdletWrite(StreamType streamType, object data, AsyncCommand writeCommand)
        {
            switch (streamType)
            {
                case StreamType.Debug:
                    writeCommand.PsCmdlet.WriteDebug((string)data);
                    break;
                case StreamType.Verbose:
                    writeCommand.PsCmdlet.WriteVerbose((string)data);
                    break;
                case StreamType.Warning:
                    writeCommand.PsCmdlet.WriteWarning((string)data);
                    break;
                case StreamType.Error:
                    writeCommand.PsCmdlet.WriteError((ErrorRecord)data);
                    break;
                case StreamType.Progress:
                    // If the activity is already completed don't write progress.
                    var progressRecord = (ProgressRecord)data;
                    if (this.progressRecords[progressRecord.ActivityId] == ProgressRecordType.Processing)
                    {
                        writeCommand.PsCmdlet.WriteProgress(progressRecord);
                    }

                    break;
                case StreamType.Object:
                    writeCommand.PsCmdlet.WriteObject(data);
                    break;
                case StreamType.Information:
                    writeCommand.PsCmdlet.WriteInformation(data, WriteInformationTags);
                    break;
            }
        }

        private class QueuedStream
        {
            public QueuedStream(StreamType type, object data)
            {
                this.Type = type;
                this.Data = data;
            }

            public StreamType Type { get; }

            public object Data { get; }
        }
    }
}
