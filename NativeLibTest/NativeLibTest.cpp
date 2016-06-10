// NativeLibTest.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "NativeLibTest.h"
#include "DXGI.h"
#include "DXGI1_2.h"
#include "D3D11.h"

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")

NATIVELIBTEST_API int grabber_get_next_frame(struct DX11ScreenGrabber *grabber, ID3D11Resource *unused);

struct DX11ScreenGrabber {
	IDXGIFactory1 *factory1;
	IDXGIAdapter1 *adapter;
	IDXGIOutput *output;
	IDXGIOutput1 *output1;

	ID3D11Device *device;
	ID3D11DeviceContext *context;
	IDXGIOutputDuplication *duplication;

	int width;
	int height;

	ID3D11Texture2D *dest_tex;
	ID3D11ShaderResourceView *dest_view;
};

int grabber_create_dest_texture(struct DX11ScreenGrabber *grabber)
{
	D3D11_TEXTURE2D_DESC desc;
	HRESULT res;

	desc.Width = grabber->width;
	desc.Height = grabber->height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	//desc.Usage = D3D11_USAGE_STAGING;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	//desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc.CPUAccessFlags = 0;
	//desc.MiscFlags = D3D11_RESOURCE_MISC_GDI_COMPATIBLE;
	desc.MiscFlags = 0;

	
	D3D11_SUBRESOURCE_DATA initial;
	initial.SysMemPitch = desc.Width * 4;
	initial.SysMemSlicePitch = desc.Width * desc.Height * 4;
	char *buf = (char *)malloc(initial.SysMemSlicePitch);
	memset(buf, 0x3f, initial.SysMemSlicePitch/2);
	initial.pSysMem = buf;

	res = grabber->device->CreateTexture2D(&desc, &initial, &grabber->dest_tex);
	if (res != S_OK) {
		return -1;
	}
	while (grabber_get_next_frame(grabber, NULL))
		;
	grabber->context->Flush();
	res = grabber->device->CreateShaderResourceView(grabber->dest_tex, NULL, &grabber->dest_view);
	if (res != S_OK) {
		return -2;
	}

	return 0;
}

NATIVELIBTEST_API void grabber_destroy(struct DX11ScreenGrabber *grabber)
{
	if (!grabber)
		return;

	if (grabber->factory1)
		grabber->factory1->Release();
	if (grabber->adapter)
		grabber->adapter->Release();
	if (grabber->output)
		grabber->output->Release();
	if (grabber->output1)
		grabber->output1->Release();

	if (grabber->device)
		grabber->device->Release();
	if (grabber->context)
		grabber->context->Release();
	if (grabber->duplication)
		grabber->duplication->Release();

	if (grabber->dest_tex)
		grabber->dest_tex->Release();
	if (grabber->dest_view)
		grabber->dest_view->Release();
}

NATIVELIBTEST_API struct DX11ScreenGrabber *grabber_create(ID3D11Resource *tex)
{
	struct DX11ScreenGrabber *grabber;
	HRESULT res;
	int r;
	int *ret = &r;

	grabber = (DX11ScreenGrabber*)malloc(sizeof(*grabber));
	memset(grabber, 0, sizeof(*grabber));

	res = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void **)&grabber->factory1);
	if (res != S_OK) {
		*ret = -1;
		goto err;
	}

	res = grabber->factory1->EnumAdapters1(0, &grabber->adapter);
	if (res != S_OK) {
		*ret = -2;
		goto err;
	}

	res = grabber->adapter->EnumOutputs(0, &grabber->output);
	if (res != S_OK) {
		*ret = -3;
		goto err;
	}

	res = grabber->output->QueryInterface(__uuidof(IDXGIOutput1), (void **)&grabber->output1);
	if (res != S_OK) {
		*ret = -4;
		goto err;
	}

	tex->GetDevice(&grabber->device);
	grabber->device->GetImmediateContext(&grabber->context);
	
	/*
	res = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_BGRA_SUPPORT, NULL, 0, D3D11_SDK_VERSION, &grabber->device, NULL, &grabber->context);
	if (res != S_OK) {
		*ret = -5;
		goto err;
	}
	*/
	

	res = grabber->output1->DuplicateOutput(grabber->device, &grabber->duplication);
	if (res != S_OK) {
		*ret = -6;
		goto err;
	}
	
	DXGI_OUTPUT_DESC outputdesc;
	res = grabber->output->GetDesc(&outputdesc);
	if (res != S_OK) {
		*ret = -7;
		goto err;
	}
	grabber->width = outputdesc.DesktopCoordinates.right - outputdesc.DesktopCoordinates.left;
	grabber->height = outputdesc.DesktopCoordinates.bottom - outputdesc.DesktopCoordinates.top;

	if (grabber_create_dest_texture(grabber)) {
		*ret = -8;
		goto err;
	}

	return grabber;
err:
	grabber_destroy(grabber);
	return NULL;
}

NATIVELIBTEST_API int grabber_get_next_frame(struct DX11ScreenGrabber *grabber, ID3D11Resource *dest)
{
	DXGI_OUTDUPL_FRAME_INFO info;
	IDXGIResource *resource;
	ID3D11Texture2D *source = NULL;
	HRESULT res;
	int ret = 0;
	
	res = grabber->duplication->AcquireNextFrame(0, &info, &resource);

	if (res != S_OK)
		return -1;

	if (!info.AccumulatedFrames) {
		ret = 1;
		goto out;
	}

	res = resource->QueryInterface(__uuidof(ID3D11Texture2D), (void **)&source);
	if (res != S_OK) {
		ret = -2;
		goto out;
	}

	D3D11_TEXTURE2D_DESC s_desc;
	source->GetDesc(&s_desc);

	D3D11_TEXTURE2D_DESC d_desc;
	grabber->dest_tex->GetDesc(&d_desc);

	grabber->context->CopyResource(grabber->dest_tex, source);
	//grabber->context->Flush();
	//grabber->context->CopyResource(dest, source);

out:
	if (source)
		source->Release();
	resource->Release();
	grabber->duplication->ReleaseFrame();

	return ret;
}

NATIVELIBTEST_API int grabber_get_width(struct DX11ScreenGrabber *grabber)
{
	return grabber->width;
}

NATIVELIBTEST_API int grabber_get_height(struct DX11ScreenGrabber *grabber)
{
	return grabber->height;
}

NATIVELIBTEST_API ID3D11ShaderResourceView *grabber_get_dest_tex(struct DX11ScreenGrabber *grabber)
{
	return grabber->dest_view;
}