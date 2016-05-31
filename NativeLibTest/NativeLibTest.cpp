// NativeLibTest.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "NativeLibTest.h"
#include "DXGI.h"
#include "DXGI1_2.h"

#pragma comment(lib, "dxgi.lib")

// This is an example of an exported function.
NATIVELIBTEST_API int fnNativeLibTest(void)
{
	HRESULT res;
	int ret = 1;

	IDXGIFactory1 *factory1 = NULL;
	IDXGIAdapter1 *adapter = NULL;
	IDXGIOutput *output = NULL;
	IDXGIOutput1 *output1 = NULL;

	res = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void **)&factory1);
	if (res != S_OK) {
		ret = -1;
		goto err;
	}
	
	res = factory1->EnumAdapters1(0, &adapter);
	if (res != S_OK) {
		ret = -2;
		goto err;
	}

	res = adapter->EnumOutputs(0, &output);
	if (res != S_OK) {
		ret = -3;
		goto err;
	}

	res = output->QueryInterface(__uuidof(IDXGIOutput1), (void **)&output1);
	if (res != S_OK) {
		ret = -4;
		goto err;
	}

	//output1->DuplicateOutput()
err:
	if (factory1)
		factory1->Release();
	if (adapter)
		adapter->Release();
	if (output)
		output->Release();
	if (output1)
		output1->Release();
	return ret;
}

// This is the constructor of a class that has been exported.
// see NativeLibTest.h for the class definition
CNativeLibTest::CNativeLibTest()
{
    return;
}
