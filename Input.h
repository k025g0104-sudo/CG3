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

private:

	ComPtr<IDirectInputDevice8> keyboard;
};

