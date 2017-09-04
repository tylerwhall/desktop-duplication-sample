// NativeLibTest.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "NativeLibTest.h"


// This is an example of an exported variable
NATIVELIBTEST_API int nNativeLibTest=0;

// This is an example of an exported function.
NATIVELIBTEST_API int fnNativeLibTest(void)
{
    return 42;
}

// This is the constructor of a class that has been exported.
// see NativeLibTest.h for the class definition
CNativeLibTest::CNativeLibTest()
{
    return;
}
