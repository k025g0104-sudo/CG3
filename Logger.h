#pragma once
#include <string>
#include <Windows.h>

// 出力ウィンドウへログを出す
inline void Log(const std::string& message)
{
	OutputDebugStringA(message.c_str());
}