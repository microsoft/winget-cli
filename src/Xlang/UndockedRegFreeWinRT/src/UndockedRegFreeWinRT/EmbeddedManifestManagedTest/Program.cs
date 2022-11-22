using System;
using System.Runtime.InteropServices;
using TestComponent;
using EmbeddedTestComponent;
using Microsoft.Windows;

namespace EmbeddedManifestManagedTest
{
    class Program
    {
        public static bool succeeded;
        private static int testsRan;
        private static int testsFailed;

        static void TestComponentClassBoth(int expected)
        {
            try
            {
                TestComponent.ClassBoth c = new TestComponent.ClassBoth();
                succeeded = (expected == c.Apartment);
            }
            catch
            {
                succeeded = false;
            }
        }

        static void TestComponentClassMta(int expected)
        {
            try
            {
                TestComponent.ClassMta c = new TestComponent.ClassMta();
                succeeded = (expected == c.Apartment);
            }
            catch
            {
                succeeded = false;
            }
        }

        static void TestComponentClassSta(int expected)
        {
            try
            {
                TestComponent.ClassSta c = new TestComponent.ClassSta();
                succeeded = (expected == c.Apartment);
            }
            catch
            {
                succeeded = false;
            }
        }

        static void EmbeddedTestComponentClassBoth(int expected)
        {
            try
            {
                EmbeddedTestComponent.ClassBoth c = new EmbeddedTestComponent.ClassBoth();
                succeeded = (expected == c.Apartment);
            }
            catch
            {
                succeeded = false;
            }
        }

        static void EmbeddedTestComponentClassMta(int expected)
        {
            try
            {
                EmbeddedTestComponent.ClassMta c = new EmbeddedTestComponent.ClassMta();
                succeeded = (expected == c.Apartment);
            }
            catch
            {
                succeeded = false;
            }
        }

        static void EmbeddedTestComponentClassSta(int expected)
        {
            try
            {
                EmbeddedTestComponent.ClassSta c = new EmbeddedTestComponent.ClassSta();
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
            Console.WriteLine("Undocked RegFree Embedded Manifest Managed Test - Starting");

            if (!RunTest(new System.Threading.ThreadStart(() => TestComponentClassBoth(3)), System.Threading.ApartmentState.STA))
            {
                Console.WriteLine("TestComponent Both to STA test failed");
                testsFailed++;
            }
            if (!RunTest(new System.Threading.ThreadStart(() => TestComponentClassBoth(1)), System.Threading.ApartmentState.MTA))
            {
                Console.WriteLine("TestComponent Both to MTA test failed");
                testsFailed++;
            }
            if (!RunTest(new System.Threading.ThreadStart(() => TestComponentClassMta(1)), System.Threading.ApartmentState.STA))
            {
                Console.WriteLine("TestComponent MTA to STA test failed");
                testsFailed++;
            }
            if (RunTest(new System.Threading.ThreadStart(() => TestComponentClassSta(1)), System.Threading.ApartmentState.MTA))
            {
                Console.WriteLine("TestComponent STA to MTA should failed");
                testsFailed++;
            }

            if (!RunTest(new System.Threading.ThreadStart(() => EmbeddedTestComponentClassBoth(3)), System.Threading.ApartmentState.STA))
            {
                Console.WriteLine("EmbeddedTestComponent Both to STA test failed");
                testsFailed++;
            }
            if (!RunTest(new System.Threading.ThreadStart(() => EmbeddedTestComponentClassBoth(1)), System.Threading.ApartmentState.MTA))
            {
                Console.WriteLine("EmbeddedTestComponent Both to MTA test failed");
                testsFailed++;
            }
            if (!RunTest(new System.Threading.ThreadStart(() => EmbeddedTestComponentClassMta(1)), System.Threading.ApartmentState.STA))
            {
                Console.WriteLine("EmbeddedTestComponent MTA to STA test failed");
                testsFailed++;
            }
            if (RunTest(new System.Threading.ThreadStart(() => EmbeddedTestComponentClassSta(1)), System.Threading.ApartmentState.MTA))
            {
                Console.WriteLine("EmbeddedTestComponent STA to MTA should failed");
                testsFailed++;
            }

            Console.WriteLine("Undocked RegFree Embedded Manifest Managed Test - " + (testsRan - testsFailed) + " out of " + testsRan + " tests passed");
            return testsFailed == 0 ? 0 : 1;
        }
    }
}
