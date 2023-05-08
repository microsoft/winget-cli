// -----------------------------------------------------------------------------
// <copyright file="AsyncCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Engine.Commands
{
    using System;
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
        private readonly CancellationToken cancellationToken;

        private readonly bool isDebugBounded;

        private Action? mainThreadAction = null;
        private bool canWriteToStream;

        /// <summary>
        /// Initializes a new instance of the <see cref="AsyncCommand"/> class.
        /// </summary>
        /// <param name="psCmdlet">PSCmdlet.</param>
        /// <param name="cancellationToken">Cancellation token.</param>
        /// <param name="canWriteToStream">If the command can write to stream.</param>
        public AsyncCommand(PSCmdlet psCmdlet, CancellationToken cancellationToken, bool canWriteToStream)
        {
            this.PsCmdlet = psCmdlet;
            this.originalThread = Thread.CurrentThread;
            this.cancellationToken = cancellationToken;
            this.isDebugBounded = this.PsCmdlet.MyInvocation.BoundParameters.ContainsKey("Debug");
            this.canWriteToStream = canWriteToStream;
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
        /// Execute the delegate in a MTA thread.
        /// </summary>
        /// <param name="func">Function to execute.</param>
        /// <returns>A <see cref="Task"/> representing the asynchronous operation.</returns>
        public Task RunOnMTA(Func<Task> func)
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
        public Task<TResult> RunOnMTA<TResult>(Func<Task<TResult>> func)
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
        public void Wait(Task runningTask)
        {
            // This must be called in the main thread.
            if (this.originalThread != Thread.CurrentThread)
            {
                throw new InvalidOperationException();
            }

            this.canWriteToStream = true;
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
        public void WriteDebug(string text)
        {
            if (!this.CanWriteToStream)
            {
                return;
            }

            // Don't do context switch if no need.
            if (!this.isDebugBounded)
            {
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
        /// Calls cmdlet WriteDebug.
        /// If its executed on the main thread calls it directly. Otherwise
        /// sets it to the main thread action and wait for it to be executed.
        /// </summary>
        /// <param name="text">Warning text.</param>
        public void WriteWarning(string text)
        {
            if (!this.CanWriteToStream)
            {
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
        /// Calls cmdlet WriteObject.
        /// </summary>
        /// <param name="obj">Object to write.</param>
        public void WriteObject(object obj)
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
    }
}
