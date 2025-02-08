#include <iostream>
#include <vcclr.h>
#include "pch.h"


using namespace System;
using namespace System::Runtime::InteropServices;

public ref class ByteArrayTransferWrapper
{
public:
    static void TransferBytes(array<Byte>^ data, int length)
    {
        // Call the C# method
        MyLibrary::ByteArrayTransfer::TransferBytes(data, length);
    }
};

extern "C" __declspec(dllexport) void __stdcall TransferBytesFromCpp(unsigned char* data, int length)
{
    array<Byte>^ managedArray = gcnew array<Byte>(length);
    Marshal::Copy(IntPtr(data), managedArray, 0, length);
    ByteArrayTransferWrapper::TransferBytes(managedArray, length);
}