using System;
using System.Runtime.InteropServices;

namespace cs_test
{
    public static class MyClass
    {
        // Import the C++ function using P/Invoke
        [DllImport("cpp_cs.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int Add(int a, int b);

        static void Main()
        {
            int result = Add(5, 3);
            Console.WriteLine("The result is: " + result);
        }

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        public delegate void MyCSharpDelegate(byte[] data, int length);

        [DllExport("MyCSharpFunction", CallingConvention = CallingConvention.StdCall)]
        public static void MyCSharpFunction(byte[] data, int length)
        {
            Console.WriteLine("Data from C++: " + BitConverter.ToString(data));
            // Example: Modify the data
            for (int i = 0; i < data.Length; i++)
            {
                data[i] = (byte)(data[i] + 1);
            }
        }
    }
}