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

    /// <summary>
    /// This is the base class for any command that performs async operations.
    /// It supports running tasks in an MTA thread via RunOnMta.
    /// If the thread is already running on an MTA it will executed it, otherwise
    /// it will create a new MTA thread.
    ///
    /// Calling PSCmdlet functions to write into their stream from not the main thread will
    /// throw an exception.
    /// This class contains wrappers around those methods with synchronization mechanisms
    /// to output the messages.
    ///
    /// Wait must be used to synchronously wait con the task.
    /// </summary>
    public abstract class AsyncCommand
    {
        private static readonly object CmdletLock = new ();

        private readonly Thread originalThread;

        private readonly SemaphoreSlim semaphore = new (1, 1);
        private readonly ManualResetEventSlim mainThreadActionReady = new (false);
        private readonly ManualResetEventSlim mainThreadActionCompleted = new (false);

        private readonly CancellationTokenSource source = new ();

        private readonly bool isDebugBounded;

        private Action? mainThreadAction = null;
        private bool canWriteToStream;
        private CancellationToken cancellationToken;
        private ConcurrentQueue<QueuedOutputStream> queuedOutputStreams = new ();

        private int progressActivityId = 0;
        private ConcurrentDictionary<int, ProgressRecordType> progressRecords = new ();

        /// <summary>
        /// Initializes a new instance of the <see cref="AsyncCommand"/> class.
        /// </summary>
        /// <param name="psCmdlet">PSCmdlet.</param>
        /// <param name="canWriteToStream">If the command can write to stream.</param>
        public AsyncCommand(PSCmdlet psCmdlet, bool canWriteToStream)
        {
            this.PsCmdlet = psCmdlet;
            this.originalThread = Thread.CurrentThread;
            this.isDebugBounded = this.PsCmdlet.MyInvocation.BoundParameters.ContainsKey("Debug");
            this.canWriteToStream = canWriteToStream;
            this.cancellationToken = this.source.Token;
        }

        private enum OutputStreamType
        {
            Debug,
            Verbose,
            Warning,
            Error,
            Progress,
        }

        /// <summary>
        /// Gets the base cmdlet.
        /// </summary>
        protected PSCmdlet PsCmdlet { get; private set; }

        /// <summary>
        /// Gets or sets a value indicating whether if writing to stream is blocked.
        /// Writing to streams must be blocked for Start-* cmdlets. The Complete-* cmdlet counterpart
        /// will enable writing to the stream when executed.
        /// TODO: For now any messages before the Complete-* call get lost. We can add a ConcurrentQueue
        /// to store message and flush them in or before the Wait call.
        /// </summary>
        private bool CanWriteToStream
        {
            get
            {
                lock (CmdletLock)
                {
                    return this.canWriteToStream;
                }
            }

            set
            {
                lock (CmdletLock)
                {
                    this.canWriteToStream = value;
                }
            }
        }

        /// <summary>
        /// Cancel this operation.
        /// </summary>
        public virtual void Cancel()
        {
            this.source.Cancel();
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
                this.WriteDebug("Already running on MTA");
                return func();
            }

            this.WriteDebug("Creating MTA thread");
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
                this.WriteDebug("Already running on MTA");
                return func();
            }

            this.WriteDebug("Creating MTA thread");
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
        internal void Wait(Task runningTask)
        {
            // This must be called in the main thread.
            if (this.originalThread != Thread.CurrentThread)
            {
                throw new InvalidOperationException();
            }

            this.Flush();

            do
            {
                // Wait for the running task to be completed or if there's
                // an action that needs to be executed in the main thread.
                WaitHandle.WaitAny(new[]
                {
                    this.mainThreadActionReady.WaitHandle,
                    ((IAsyncResult)runningTask).AsyncWaitHandle,
                });

                if (this.mainThreadActionReady.IsSet)
                {
                    // Someone needs the main thread.
                    this.mainThreadActionReady.Reset();

                    if (this.mainThreadAction != null)
                    {
                        this.mainThreadAction();
                    }

                    // Done.
                    this.mainThreadActionCompleted.Set();
                }
            }
            while (!runningTask.IsCompleted);

            if (runningTask.IsFaulted)
            {
                // If IsFaulted is true, the task's Status is equal to Faulted,
                // and its Exception property will be non-null.
                throw runningTask.Exception!;
            }
        }

        /// <summary>
        /// Calls cmdlet WriteDebug.
        /// If its executed on the main thread calls it directly. Otherwise
        /// sets it to the main thread action and wait for it to be executed.
        /// </summary>
        /// <param name="text">Debug text.</param>
        internal void WriteDebug(string text)
        {
            // Don't do context switch if no need.
            if (!this.isDebugBounded)
            {
                return;
            }

            if (!this.CanWriteToStream)
            {
                this.queuedOutputStreams.Enqueue(
                    new QueuedOutputStream(OutputStreamType.Debug, text));
                return;
            }

            if (this.originalThread == Thread.CurrentThread)
            {
                this.PsCmdlet.WriteDebug(text);
                return;
            }

            try
            {
                this.WaitForOurTurn();
                this.mainThreadAction = () => this.PsCmdlet.WriteDebug(text);
                this.mainThreadActionReady.Set();
                this.WaitMainThreadActionCompletion();
            }
            catch (Exception)
            {
                throw;
            }
        }

        /// <summary>
        /// Calls cmdlet WriteWarning.
        /// If its executed on the main thread calls it directly. Otherwise
        /// sets it to the main thread action and wait for it to be executed.
        /// </summary>
        /// <param name="text">Warning text.</param>
        internal void WriteWarning(string text)
        {
            if (!this.CanWriteToStream)
            {
                this.queuedOutputStreams.Enqueue(
                    new QueuedOutputStream(OutputStreamType.Warning, text));
                return;
            }

            if (this.originalThread == Thread.CurrentThread)
            {
                this.PsCmdlet.WriteWarning(text);
                return;
            }

            try
            {
                this.WaitForOurTurn();
                this.mainThreadAction = () => this.PsCmdlet.WriteWarning(text);
                this.mainThreadActionReady.Set();
                this.WaitMainThreadActionCompletion();
            }
            catch (Exception)
            {
                throw;
            }
        }

        /// <summary>
        /// Calls cmdlet WriteError.
        /// If its executed on the main thread calls it directly. Otherwise
        /// sets it to the main thread action and wait for it to be executed.
        /// </summary>
        /// <param name="errorRecord">Error record.</param>
        internal void WriteError(ErrorRecord errorRecord)
        {
            if (!this.CanWriteToStream)
            {
                this.queuedOutputStreams.Enqueue(
                    new QueuedOutputStream(OutputStreamType.Error, errorRecord));
                return;
            }

            if (this.originalThread == Thread.CurrentThread)
            {
                this.PsCmdlet.WriteError(errorRecord);
                return;
            }

            try
            {
                this.WaitForOurTurn();
                this.mainThreadAction = () => this.PsCmdlet.WriteError(errorRecord);
                this.mainThreadActionReady.Set();
                this.WaitMainThreadActionCompletion();
            }
            catch (Exception)
            {
                throw;
            }
        }

        /// <summary>
        /// Calls cmdlet WriteProgress.
        /// </summary>
        /// <param name="progressRecord">Progress record.</param>
        internal void WriteProgress(ProgressRecord progressRecord)
        {
            // Keep track of all progress activity.
            if (!this.progressRecords.TryAdd(progressRecord.ActivityId, progressRecord.RecordType))
            {
                _ = this.progressRecords.TryUpdate(progressRecord.ActivityId, progressRecord.RecordType, ProgressRecordType.Completed);
            }

            if (!this.CanWriteToStream)
            {
                this.queuedOutputStreams.Enqueue(
                    new QueuedOutputStream(OutputStreamType.Progress, progressRecord));
                return;
            }

            if (this.originalThread == Thread.CurrentThread)
            {
                this.PsCmdlet.WriteProgress(progressRecord);
                return;
            }

            try
            {
                this.WaitForOurTurn();
                this.mainThreadAction = () => this.PsCmdlet.WriteProgress(progressRecord);
                this.mainThreadActionReady.Set();
                this.WaitMainThreadActionCompletion();
            }
            catch (Exception)
            {
                throw;
            }
        }

        /// <summary>
        /// Calls cmdlet WriteObject.
        /// </summary>
        /// <param name="obj">Object to write.</param>
        internal void WriteObject(object obj)
        {
            if (this.originalThread == Thread.CurrentThread)
            {
                this.PsCmdlet.WriteObject(obj);
                return;
            }

            try
            {
                this.WaitForOurTurn();
                this.mainThreadAction = () => this.PsCmdlet.WriteObject(obj);
                this.mainThreadActionReady.Set();
                this.WaitMainThreadActionCompletion();
            }
            catch (Exception)
            {
                throw;
            }
        }

        /// <summary>
        /// Enable writing to pwsh streams and flush all the queued stream.
        /// This method must be called in the original thread.
        /// WARNING: You must only call this when the task is completed or in Wait.
        /// </summary>
        internal void Flush()
        {
            // This must be called in the main thread.
            if (this.originalThread != Thread.CurrentThread)
            {
                throw new InvalidOperationException();
            }

            this.canWriteToStream = true;

            // At this point, no new messages should be added to the queue and we are in the main thread.
            // Any non completed async operation will now wait for the main thread, so be sure to cancel
            // if anything goes wrong.
            try
            {
                while (this.queuedOutputStreams.TryDequeue(out var queuedOutput))
                {
                    if (queuedOutput != null)
                    {
                        switch (queuedOutput.Type)
                        {
                            case OutputStreamType.Debug:
                                this.WriteDebug((string)queuedOutput.Data);
                                break;
                            case OutputStreamType.Warning:
                                this.WriteWarning((string)queuedOutput.Data);
                                break;
                            case OutputStreamType.Error:
                                this.WriteError((ErrorRecord)queuedOutput.Data);
                                break;
                            case OutputStreamType.Progress:
                                // If the activity is already completed don't write progress.
                                var progressRecord = (ProgressRecord)queuedOutput.Data;
                                if (this.progressRecords[progressRecord.ActivityId] == ProgressRecordType.Processing)
                                {
                                    this.WriteProgress(progressRecord);
                                }

                                break;
                        }
                    }
                }
            }
            catch (Exception)
            {
                this.Cancel();
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

        private void WaitForOurTurn()
        {
            this.semaphore.Wait(this.cancellationToken);
            this.mainThreadActionCompleted.Reset();
        }

        private void WaitMainThreadActionCompletion()
        {
            WaitHandle.WaitAny(new[]
            {
                this.cancellationToken.WaitHandle,
                this.mainThreadActionCompleted.WaitHandle,
            });

            this.semaphore.Release();
        }

        private class QueuedOutputStream
        {
            public QueuedOutputStream(OutputStreamType type, object data)
            {
                this.Type = type;
                this.Data = data;
            }

            public OutputStreamType Type { get; }

            public object Data { get; }
        }
    }
}
