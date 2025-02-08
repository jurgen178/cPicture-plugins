namespace MyLibrary
{
    public class ByteArrayTransfer
    {
        public static void TransferBytes(byte[] data, int length)
        {
            // Process the byte array
            Console.WriteLine("Received byte array of length: " + length);
            foreach (var b in data)
            {
                Console.Write(b + " ");
            }
            Console.WriteLine();
        }
    }
}