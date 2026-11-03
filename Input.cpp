#include "Input.h"

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

void Input::Initialize(WinApp* winApp)
{
	HRESULT result;

	this->winApp = winApp;

	result = DirectInput8Create(winApp->GetHInstance(), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&directInput, nullptr);
	assert(SUCCEEDED(result));

	//ComPtr<IDirectInputDevice8> keyboard = nullptr;
	result = directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
	assert(SUCCEEDED(result));

	result = keyboard->SetDataFormat(&c_dfDIKeyboard);
	assert(SUCCEEDED(result));

	result = keyboard->SetCooperativeLevel(winApp->GetHwnd(), DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
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
	return key[keyNumber] != 0;
}

bool Input::TriggerKey(BYTE keyNumber)
{
	// 今回は押されていて、前回は押されていない
	return key[keyNumber] != 0 && keyPrev[keyNumber] == 0;
}