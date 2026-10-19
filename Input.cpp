#include "Input.h"

#include <cstdint>
#include <string>
#include <cassert>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <format>
#include <dxgidebug.h>
#include <dxcapi.h>
#include "externals/DirectXTex/DirectXTex.h"
#include <fstream>
#include <sstream>
#include <algorithm> 
#include <xaudio2.h>
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"dxcompiler.lib")
#pragma comment(lib,"xaudio2.lib")

#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"dxguid.lib")

using namespace Microsoft::WRL;


void Input::Initialize(HINSTANCE hInstance, HWND hwnd)
{
	HRESULT result;


	result = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&directInput, nullptr);
	assert(SUCCEEDED(result));

	//ComPtr<IDirectInputDevice8> keyboard = nullptr;
	result = directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
	assert(SUCCEEDED(result));

	result = keyboard->SetDataFormat(&c_dfDIKeyboard);
	assert(SUCCEEDED(result));

	result = keyboard->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
	assert(SUCCEEDED(result));
}

void Input::Update()
{
	

	memcpy(keyPrev, key, sizeof(key));

	keyboard->Acquire();
	
	keyboard->GetDeviceState(sizeof(key), key);



}

bool Input::PushKey(BYTE keyNumber)
{

	if (key[keyNumber])
	{
		return true;
	}

	return false;
}

bool Input::TrigerKey(BYTE keyNumber)
{
	
	if (key[keyNumber])
	{
		return true;
	}
	else
	{
		return false;
	}

	return false;
}
