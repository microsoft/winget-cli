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
    /// Wait must be used to synchronously wait con the task.
    /// </summary>
    public abstract class AsyncCommand
    {
        private readonly Thread originalThread;

        private readonly CancellationTokenSource source = new ();

        private CancellationToken cancellationToken;
        private BlockingCollection<QueuedOutputStream> queuedOutputStreams = new ();

        private int progressActivityId = 0;
        private ConcurrentDictionary<int, ProgressRecordType> progressRecords = new ();

        /// <summary>
        /// Initializes a new instance of the <see cref="AsyncCommand"/> class.
        /// </summary>
        /// <param name="psCmdlet">PSCmdlet.</param>
        public AsyncCommand(PSCmdlet psCmdlet)
        {
            this.PsCmdlet = psCmdlet;
            this.originalThread = Thread.CurrentThread;
            this.cancellationToken = this.source.Token;
        }

        private enum OutputStreamType
        {
            Debug,
            Verbose,
            Warning,
            Error,
            Progress,
            Object,
        }

        /// <summary>
        /// Gets the base cmdlet.
        /// </summary>
        protected PSCmdlet PsCmdlet { get; private set; }

        /// <summary>
        /// Cancel this operation.
        /// </summary>
        public virtual void Complete()
        {
            this.queuedOutputStreams.CompleteAdding();
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

            do
            {
                this.ConsumeStreams();
            }
            while (!runningTask.IsCompleted && this.queuedOutputStreams.IsCompleted);

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
            if (this.originalThread == Thread.CurrentThread)
            {
                this.PsCmdlet.WriteDebug(text);
            }
            else
            {
                this.queuedOutputStreams.Add(new QueuedOutputStream(OutputStreamType.Debug, text));
            }
        }

        /// <summary>
        /// Calls cmdlet WriteVerbose.
        /// If its executed on the main thread calls it directly. Otherwise
        /// sets it to the main thread action and wait for it to be executed.
        /// </summary>
        /// <param name="text">Verbose text.</param>
        internal void WriteVerbose(string text)
        {
            if (this.originalThread == Thread.CurrentThread)
            {
                this.PsCmdlet.WriteVerbose(text);
            }
            else
            {
                this.queuedOutputStreams.Add(new QueuedOutputStream(OutputStreamType.Verbose, text));
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
            if (this.originalThread == Thread.CurrentThread)
            {
                this.PsCmdlet.WriteWarning(text);
            }
            else
            {
                this.queuedOutputStreams.Add(new QueuedOutputStream(OutputStreamType.Warning, text));
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
            if (this.originalThread == Thread.CurrentThread)
            {
                this.PsCmdlet.WriteError(errorRecord);
            }
            else
            {
                this.queuedOutputStreams.Add(new QueuedOutputStream(OutputStreamType.Error, errorRecord));
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

            if (this.originalThread == Thread.CurrentThread)
            {
                this.PsCmdlet.WriteProgress(progressRecord);
            }
            else
            {
                this.queuedOutputStreams.Add(new QueuedOutputStream(OutputStreamType.Progress, progressRecord));
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
            }
            else
            {
                this.queuedOutputStreams.Add(new QueuedOutputStream(OutputStreamType.Object, obj));
            }
        }

        /// <summary>
        /// Writes to PowerShell streams.
        /// This method must be called in the original thread.
        /// WARNING: You must only call this when the task is completed or in Wait.
        /// </summary>
        internal void ConsumeStreams()
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
                    var queuedOutput = this.queuedOutputStreams.Take();
                    switch (queuedOutput.Type)
                    {
                        case OutputStreamType.Debug:
                            this.WriteDebug((string)queuedOutput.Data);
                            break;
                        case OutputStreamType.Verbose:
                            this.WriteVerbose((string)queuedOutput.Data);
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
                        case OutputStreamType.Object:
                            this.WriteObject(queuedOutput.Data);

                            break;
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
