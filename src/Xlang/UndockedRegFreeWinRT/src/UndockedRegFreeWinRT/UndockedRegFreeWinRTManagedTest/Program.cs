using System;
using System.Runtime.InteropServices;
using TestComponent;
using Microsoft.Windows;

namespace UndockedRegFreeWinRTManagedTest
{
    class Program
    {
        public static bool succeeded;
        private static int testsRan;
        private static int testsFailed;

        static void TestClassBoth(int expected)
        {
            try
            {
                TestComponent.ClassBoth c = new ClassBoth();
                succeeded = (expected == c.Apartment);
            }
            catch
            {
                succeeded = false;
            }
        }

        static void TestClassMta(int expected)
        {
            try
            {
                TestComponent.ClassMta c = new ClassMta();
                succeeded = (expected == c.Apartment);
            }
            catch
            {
                succeeded = false;
            }
        }

        static void TestClassSta(int expected)
        {
            try
            {
                TestComponent.ClassSta c = new ClassSta();
                succeeded = (expected == c.Apartment);
            }
            catch
            {
                succeeded = false;
            }
        }

        static bool RunTest(System.Threading.ThreadStart testThreadStart, System.Threading.ApartmentState apartmentState)
        {
            testsRan++;
            System.Threading.Thread testThread = new System.Threading.Thread(testThreadStart);
            succeeded = false;
            testThread.SetApartmentState(apartmentState);
            testThread.Start();
            testThread.Join();
            return succeeded;
        }

        static int Main(string[] args)
        {
            UndockedRegFreeWinrt.Initialize();
            Console.WriteLine("Undocked RegFree WinRT Managed Test - Starting");

            if (!RunTest(new System.Threading.ThreadStart(() => TestClassBoth(3)), System.Threading.ApartmentState.STA))
            {
                Console.WriteLine("Both to STA test failed");
                testsFailed++;
            }

            if (!RunTest(new System.Threading.ThreadStart(() => TestClassBoth(1)), System.Threading.ApartmentState.MTA))
            {
                Console.WriteLine("Both to MTA test failed");
                testsFailed++;
            }

            if (!RunTest(new System.Threading.ThreadStart(() => TestClassMta(1)), System.Threading.ApartmentState.STA))
            {
                Console.WriteLine("MTA to STA test failed");
                testsFailed++;
            }

            if (RunTest(new System.Threading.ThreadStart(() => TestClassSta(1)), System.Threading.ApartmentState.MTA))
            {
                Console.WriteLine("STA to MTA should failed");
                testsFailed++;
            }

            Console.WriteLine("Undocked RegFree WinRT Managed Test - " + (testsRan - testsFailed) + " out of " + testsRan + " tests passed");
            return testsFailed == 0 ? 0 : 1;
        }
    }
}
