// -----------------------------------------------------------------------------
// <copyright file="PowerShellCmdlet.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Common.Command
{
    using System;
    using System.Collections.Concurrent;
    using System.Collections.Generic;
    using System.Management.Automation;
    using System.Runtime.ExceptionServices;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.WinGet.Resources;
    using Microsoft.WinGet.SharedLib.Exceptions;
    using Microsoft.WinGet.SharedLib.PolicySettings;

    /// <summary>
    /// This must be the base class for every cmdlet for winget PowerShell modules.
    /// It supports:
    ///  - Async operations.
    ///  - Execute on an MTA. If the thread is already running on an MTA it will executed it, otherwise
    ///    it will create a new MTA thread.
    /// Wait must be used to synchronously wait con the task.
    /// </summary>
    public abstract class PowerShellCmdlet
    {
        private const string Debug = "Debug";
        private static readonly string[] WriteInformationTags = new string[] { "PSHOST" };

        private readonly PSCmdlet psCmdlet;
        private readonly Thread pwshThread;

        private readonly CancellationTokenSource source = new ();
        private readonly SemaphoreSlim semaphore = new (1, 1);
        private readonly ManualResetEventSlim pwshThreadActionReady = new (false);
        private readonly ManualResetEventSlim pwshThreadActionCompleted = new (false);

        private BlockingCollection<QueuedStream> queuedStreams = new ();
        private int progressActivityId = 0;
        private ConcurrentDictionary<int, ProgressRecordType> progressRecords = new ();
        private Action? pwshThreadAction = null;
        private ExceptionDispatchInfo? pwshThreadEdi = null;

        /// <summary>
        /// Initializes a new instance of the <see cref="PowerShellCmdlet"/> class.
        /// </summary>
        /// <param name="psCmdlet">PSCmdlet.</param>
        /// <param name="policies">Policies.</param>
        public PowerShellCmdlet(PSCmdlet psCmdlet, HashSet<Policy> policies)
        {
            // Passing Debug will make all the message actions to be Inquire. For async operations
            // and the current queue message implementation this doesn't make sense.
            // PowerShell will inquire for any message giving the impression that the task is
            // paused, but the async operation is still running.
            if (psCmdlet.MyInvocation.BoundParameters.ContainsKey(Debug))
            {
                throw new NotSupportedException(Resources.DebugNotSupported);
            }

            this.ValidatePolicies(policies);

            this.psCmdlet = psCmdlet;
            this.pwshThread = Thread.CurrentThread;
        }

        /// <summary>
        /// Request cancellation for this command.
        /// </summary>
        public void Cancel()
        {
            this.source.Cancel();
        }

        /// <summary>
        /// Execute the delegate in a MTA thread.
        /// Caller must wait on task.
        /// </summary>
        /// <param name="func">Function to execute.</param>
        /// <returns>A <see cref="Task"/> representing the asynchronous operation.</returns>
        internal Task RunOnMTA(Func<Task> func)
        {
            // .NET 4.8 doesn't support TaskCompletionSource.
#if POWERSHELL_WINDOWS
            throw new NotImplementedException();
#else
            // This must be called in the main thread.
            if (this.pwshThread != Thread.CurrentThread)
            {
                throw new InvalidOperationException();
            }

            if (Thread.CurrentThread.GetApartmentState() == ApartmentState.MTA)
            {
                this.Write(StreamType.Verbose, "Already running on MTA");
                try
                {
                    Task result = func();
                    result.ContinueWith((task) => this.Complete(), TaskContinuationOptions.ExecuteSynchronously);
                    return result;
                }
                catch
                {
                    this.Complete();
                    throw;
                }
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
                finally
                {
                    this.Complete();
                }
            });

            thread.SetApartmentState(ApartmentState.MTA);
            thread.Start();
            return tcs.Task;
#endif
        }

        /// <summary>
        /// Execute the delegate in a MTA thread.
        /// Caller must wait on task.
        /// </summary>
        /// <param name="func">Function to execute.</param>
        /// <typeparam name="TResult">Return type of function.</typeparam>
        /// <returns>A <see cref="Task"/> representing the asynchronous operation.</returns>
        internal Task<TResult> RunOnMTA<TResult>(Func<Task<TResult>> func)
        {
            // This must be called in the main thread.
            if (this.pwshThread != Thread.CurrentThread)
            {
                throw new InvalidOperationException();
            }

            if (Thread.CurrentThread.GetApartmentState() == ApartmentState.MTA)
            {
                this.Write(StreamType.Verbose, "Already running on MTA");
                try
                {
                    Task<TResult> result = func();
                    result.ContinueWith((task) => this.Complete(), TaskContinuationOptions.ExecuteSynchronously);
                    return result;
                }
                catch
                {
                    this.Complete();
                    throw;
                }
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
                finally
                {
                    this.Complete();
                }
            });

            thread.SetApartmentState(ApartmentState.MTA);
            thread.Start();
            return tcs.Task;
        }

        /// <summary>
        /// Execute the delegate in a MTA thread.
        /// Synchronous call.
        /// </summary>
        /// <param name="func">Function to execute.</param>
        /// <typeparam name="TResult">Return type of function.</typeparam>
        /// <returns>A <see cref="Task"/> representing the asynchronous operation.</returns>
        internal TResult RunOnMTA<TResult>(Func<TResult> func)
        {
            // This must be called in the main thread.
            if (this.pwshThread != Thread.CurrentThread)
            {
                throw new InvalidOperationException();
            }

            if (Thread.CurrentThread.GetApartmentState() == ApartmentState.MTA)
            {
                this.Write(StreamType.Verbose, "Already running on MTA");
                try
                {
                    return func();
                }
                finally
                {
                    this.Complete();
                }
            }

            this.Write(StreamType.Verbose, "Creating MTA thread");
            var tcs = new TaskCompletionSource<TResult>();
            var thread = new Thread(() =>
            {
                try
                {
                    var result = func();
                    tcs.SetResult(result);
                }
                catch (Exception e)
                {
                    tcs.SetException(e);
                }
                finally
                {
                    this.Complete();
                }
            });

            thread.SetApartmentState(ApartmentState.MTA);
            thread.Start();
            this.Wait(tcs.Task);
            return tcs.Task.Result;
        }

        /// <summary>
        /// Executes an action in the main thread.
        /// Blocks until call is executed.
        /// </summary>
        /// <param name="action">Action to perform.</param>
        internal void ExecuteInPowerShellThread(Action action)
        {
            if (this.pwshThread == Thread.CurrentThread)
            {
                action();
                return;
            }

            this.WaitForOurTurn();

            this.pwshThreadAction = action;
            this.pwshThreadActionReady.Set();
            this.WaitMainThreadActionCompletion();
        }

        /// <summary>
        /// Waits for the task to be completed. This MUST be called from the main thread.
        /// </summary>
        /// <param name="runningTask">Task to wait for.</param>
        /// <param name="writeCmdlet">The cmdlet that can write to PowerShell.</param>
        internal void Wait(Task runningTask, PowerShellCmdlet? writeCmdlet = null)
        {
            writeCmdlet ??= this;

            // This must be called in the main thread.
            if (this.pwshThread != Thread.CurrentThread)
            {
                throw new InvalidOperationException();
            }

            do
            {
                if (this.pwshThreadActionReady.IsSet)
                {
                    // Someone needs the main thread.
                    this.pwshThreadActionReady.Reset();

                    if (this.pwshThreadAction != null)
                    {
                        try
                        {
                            this.pwshThreadEdi = null;
                            this.pwshThreadAction();
                        }
                        catch (Exception e)
                        {
                            // Make sure we don't throw in the PowerShell thread, this way
                            // we'll get a more meaningful stack by Get-Error.
                            this.pwshThreadEdi = ExceptionDispatchInfo.Capture(e);
                        }

                        this.pwshThreadAction = null;
                    }

                    // Done.
                    this.pwshThreadActionCompleted.Set();
                }

                // Take from the blocking collection.
                if (!this.queuedStreams.IsCompleted && this.queuedStreams.Count > 0)
                {
                    try
                    {
                        var queuedOutput = this.queuedStreams.Take();
                        if (queuedOutput != null)
                        {
                            this.CmdletWrite(queuedOutput.Type, queuedOutput.Data, writeCmdlet);
                        }
                    }
                    catch (InvalidOperationException)
                    {
                        // An InvalidOperationException means that Take() was called on a completed collection.
                    }
                }
            }
            while (!(runningTask.IsCompleted && this.queuedStreams.IsCompleted));

            if (runningTask.IsFaulted)
            {
                // If IsFaulted is true, the task's Status is equal to Faulted,
                // and its Exception property will be non-null.
                AggregateException? ae = runningTask.Exception! as AggregateException;
                if (ae != null && ae.InnerExceptions.Count == 1)
                {
                    ExceptionDispatchInfo.Capture(ae.InnerExceptions[0]).Throw();
                }

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
                ProgressRecord progressRecord = (ProgressRecord)data;
                if (progressRecord.RecordType == ProgressRecordType.Completed)
                {
                    throw new NotSupportedException("Use CompleteProgress");
                }

                // Keep track of all progress activity.
                _ = this.progressRecords.TryAdd(progressRecord.ActivityId, progressRecord.RecordType);
            }

            if (this.pwshThread == Thread.CurrentThread)
            {
                this.CmdletWrite(type, data, this);
                return;
            }

            this.queuedStreams.Add(new QueuedStream(type, data));
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
        /// <param name="force">Force write complete progress.</param>
        internal void CompleteProgress(int activityId, string activity, string status, bool force = false)
        {
            var record = new ProgressRecord(activityId, activity, status)
            {
                RecordType = ProgressRecordType.Completed,
                PercentComplete = 100,
            };

            if (!this.progressRecords.TryAdd(activityId, record.RecordType))
            {
                _ = this.progressRecords.TryUpdate(activityId, record.RecordType, ProgressRecordType.Processing);
            }

            if (this.pwshThread == Thread.CurrentThread)
            {
                this.CmdletWrite(StreamType.Progress, record, this);
            }
            else
            {
                // You should only use force if you know the cmdlet that is completing this progress is a sync cmdlet that
                // is running in an async context. A sync cmdlet is anything that doesn't start with Start-*
                if (force)
                {
                    this.ExecuteInPowerShellThread(() => this.CmdletWrite(StreamType.Progress, record, this));
                }
                else
                {
                    this.queuedStreams.Add(new QueuedStream(StreamType.Progress, record));
                }
            }
        }

        /// <summary>
        /// Writes to PowerShell streams.
        /// This method must be called in the original thread.
        /// WARNING: You must only call this when the task is completed.
        /// </summary>
        /// <param name="writeCmdlet">The cmdlet that can write to PowerShell.</param>
        internal void ConsumeAndWriteStreams(PowerShellCmdlet writeCmdlet)
        {
            // This must be called in the main thread.
            if (this.pwshThread != Thread.CurrentThread)
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
                        this.CmdletWrite(queuedOutput.Type, queuedOutput.Data, writeCmdlet);
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
        internal CancellationToken GetCancellationToken()
        {
            return this.source.Token;
        }

        /// <summary>
        /// Gets the current file system location from the cmdlet.
        /// </summary>
        /// <returns>Path.</returns>
        internal string GetCurrentFileSystemLocation()
        {
            return this.psCmdlet.SessionState.Path.CurrentFileSystemLocation.Path;
        }

        /// <summary>
        /// Sets a variable.
        /// </summary>
        /// <param name="variableName">Variable name.</param>
        /// <param name="value">Value.</param>
        internal void SetVariable(string variableName, object value)
        {
            this.psCmdlet.SessionState.PSVariable.Set(variableName, value);
        }

        /// <summary>
        /// Prompts the user if it should continue processing if possible.
        /// </summary>
        /// <param name="target">Message.</param>
        /// <returns>If the operation should continue.</returns>
        internal bool ShouldProcess(string target)
        {
            // If not on the main thread just continue.
            if (this.pwshThread != Thread.CurrentThread)
            {
                return true;
            }

            return this.psCmdlet.ShouldProcess(target);
        }

        private void Complete()
        {
            this.queuedStreams.CompleteAdding();
        }

        private void CmdletWrite(StreamType streamType, object data, PowerShellCmdlet writeCmdlet)
        {
            switch (streamType)
            {
                case StreamType.Debug:
                    throw new NotSupportedException();
                case StreamType.Verbose:
                    writeCmdlet.psCmdlet.WriteVerbose((string)data);
                    break;
                case StreamType.Warning:
                    writeCmdlet.psCmdlet.WriteWarning((string)data);
                    break;
                case StreamType.Error:
                    writeCmdlet.psCmdlet.WriteError((ErrorRecord)data);
                    break;
                case StreamType.Progress:
                    // If the activity is already completed don't write progress.
                    var progressRecord = (ProgressRecord)data;
                    if (this.progressRecords[progressRecord.ActivityId] == progressRecord.RecordType)
                    {
                        writeCmdlet.psCmdlet.WriteProgress(progressRecord);
                    }

                    break;
                case StreamType.Object:
                    writeCmdlet.psCmdlet.WriteObject(data);
                    break;
                case StreamType.Information:
                    writeCmdlet.psCmdlet.WriteInformation(data, WriteInformationTags);
                    break;
            }
        }

        private void ValidatePolicies(HashSet<Policy> policies)
        {
            GroupPolicy groupPolicy = GroupPolicy.GetInstance();

            if (policies.Contains(Policy.WinGet))
            {
                if (!groupPolicy.IsEnabled(Policy.WinGet))
                {
                    throw new GroupPolicyException(Policy.WinGet, GroupPolicyFailureType.BlockedByPolicy);
                }

                policies.Remove(Policy.WinGet);
            }

            if (policies.Contains(Policy.Configuration))
            {
                if (!groupPolicy.IsEnabled(Policy.Configuration))
                {
                    throw new GroupPolicyException(Policy.Configuration, GroupPolicyFailureType.BlockedByPolicy);
                }

                policies.Remove(Policy.Configuration);
            }

            if (policies.Contains(Policy.WinGetCommandLineInterfaces))
            {
                if (!groupPolicy.IsEnabled(Policy.WinGetCommandLineInterfaces))
                {
                    throw new GroupPolicyException(Policy.WinGetCommandLineInterfaces, GroupPolicyFailureType.BlockedByPolicy);
                }

                policies.Remove(Policy.WinGetCommandLineInterfaces);
            }

            if (policies.Count > 0)
            {
                throw new NotSupportedException($"Invalid policies {string.Join(",", policies)}");
            }
        }

        private void WaitForOurTurn()
        {
            this.semaphore.Wait(this.GetCancellationToken());
            this.pwshThreadActionCompleted.Reset();
        }

        private void WaitMainThreadActionCompletion()
        {
            WaitHandle.WaitAny(new[]
            {
                this.GetCancellationToken().WaitHandle,
                this.pwshThreadActionCompleted.WaitHandle,
            });

            try
            {
                if (this.pwshThreadEdi != null)
                {
                    this.pwshThreadEdi.Throw();
                }
            }
            finally
            {
                this.semaphore.Release();
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
