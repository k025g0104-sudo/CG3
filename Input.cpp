#include "Input.h"

#include <cassert>
#include <cstring>

#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"dxguid.lib")

void Input::Initialize(WinApp* winApp)
{
	HRESULT result;

	// 借りてきた WinApp のインスタンスを記録
	this->winApp = winApp;

	// DirectInput のインスタンス生成
	result = DirectInput8Create(winApp->GetHInstance(), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&directInput, nullptr);
	assert(SUCCEEDED(result));

	// キーボードデバイス生成
	result = directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
	assert(SUCCEEDED(result));

	// 入力データ形式のセット
	result = keyboard->SetDataFormat(&c_dfDIKeyboard);
	assert(SUCCEEDED(result));

	// 排他制御レベルのセット
	result = keyboard->SetCooperativeLevel(winApp->GetHwnd(), DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
	assert(SUCCEEDED(result));
}

void Input::Update()
{
	// 前回のキー状態を保存
	memcpy(keyPrev, key, sizeof(key));

	// キーボード情報の取得開始
	keyboard->Acquire();

	// 全キーの入力状態を取得
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