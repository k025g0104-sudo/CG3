#pragma once
#include <Windows.h>
#include <cstdint>

class WinApp
{
public:
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	void Initialize();
	void Update();

	HWND GetHwnd() const { return hwnd_; }

public:
	static const int32_t kClientWidth = 1280;
	static const int32_t kClientHeight = 720;

private:
	HWND hwnd_ = nullptr;
	WNDCLASS wc_{};
};