// -----------------------------------------------------------------------------
// <copyright file="MtaCommand.cs" company="Microsoft Corporation">
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
    /// This is the base class for any command that needs to be executed on an MTA thread.
    /// Call RunOnMTA to start any async function call. If the thread is already running
    /// on an MTA it will executed it, otherwise it will create a new MTA thread.
    ///
    /// Calling PSCmdlet functions to write into their stream from not the main thread will
    /// throw an exception.
    /// This class contains wrappers around those methods with synchronization mechanisms
    /// to output the messages.
    /// </summary>
    public abstract class MtaCommand
    {
        private readonly Thread originalThread;

        private readonly SemaphoreSlim semaphore = new (1, 1);
        private readonly ManualResetEventSlim mainThreadActionReady = new (false);
        private readonly ManualResetEventSlim mainThreadActionCompleted = new (false);
        private readonly CancellationToken cancellationToken;

        private Action? mainThreadAction = null;

        /// <summary>
        /// Initializes a new instance of the <see cref="MtaCommand"/> class.
        /// </summary>
        /// <param name="psCmdlet">PSCmdlet.</param>
        /// <param name="cancellationToken">Cancellation token.</param>
        public MtaCommand(PSCmdlet psCmdlet, CancellationToken cancellationToken)
        {
            this.PsCmdlet = psCmdlet;
            this.originalThread = Thread.CurrentThread;
            this.cancellationToken = cancellationToken;
        }

        /// <summary>
        /// Gets the base cmdlet.
        /// </summary>
        protected PSCmdlet PsCmdlet { get; private set; }

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
        /// <typeparam name="TReturn">Return type of function.</typeparam>
        /// <returns>A <see cref="Task"/> representing the asynchronous operation.</returns>
        public Task<TReturn> RunOnMTA<TReturn>(Func<Task<TReturn>> func)
            where TReturn : struct
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
            var tcs = new TaskCompletionSource<TReturn>();
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
        protected void WriteDebug(string text)
        {
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
