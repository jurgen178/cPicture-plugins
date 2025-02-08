using System.Runtime.InteropServices;

namespace cs_test
{
    public class Class1
    {
        [UnmanagedCallersOnly(EntryPoint = nameof(TestMethod))]
        public static void TestMethod() => Console.Out.WriteLine("Hello from C#");
    }
}
