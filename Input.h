#pragma once

#include<windows.h>
#include <wrl.h>
#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#endif
#include <dinput.h>

class Input
{
public:
	template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

	void Initialize(HINSTANCE hInstance,HWND hwnd);

	void Update();

	bool PushKey(BYTE keyNumber);
	bool TrigerKey(BYTE keyNumber);
private:
	
	ComPtr<IDirectInputDevice8> keyboard;
	ComPtr<IDirectInput8> directInput;
	BYTE key[256] = {};
	BYTE keyPrev[256] = {};
};

