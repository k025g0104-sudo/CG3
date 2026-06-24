#include <Windows.h>
#include <cstdint>
#include <string>
#include <cassert>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <format>
#include <dxgidebug.h>
#include <dxcapi.h>
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_win32.h"
#include "externals/imgui/imgui_impl_dx12.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif // USE_IMGUI
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"dxcompiler.lib")

struct Vector3
{
	float x, y, z;
};

struct Vector4
{
	float x, y, z, w;
};

struct Matrix4x4
{
	float m[4][4];
};

struct Transform
{
	Vector3 scale;

	Vector3 rotate;

	Vector3 translate;
};

struct CameraTransform
{
	Vector3 scale;
	
	Vector3 rotate;
	
	Vector3 translate;
};


Matrix4x4 MakeIdentity4x4()
{
	Matrix4x4 result{};
	result.m[0][0] = 1.0f;
	result.m[1][1] = 1.0f;
	result.m[2][2] = 1.0f;
	result.m[3][3] = 1.0f;
	return result;
}

Matrix4x4 MakeIdentityMatrix4x4()
{
	Matrix4x4 result =
	{
		1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
	};
	return result;
}


Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2)
{
	Matrix4x4 result = {};
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			result.m[i][j] = 0.0f;
			for (int k = 0; k < 4; k++)
			{
				result.m[i][j] += m1.m[i][k] * m2.m[k][j];
			}
		}
	}
	return result;
}


Matrix4x4 rotateX(float angle)
{
	Matrix4x4 result = MakeIdentityMatrix4x4();
	float cosA = std::cos(angle);
	float sinA = std::sin(angle);
	result.m[1][1] = cosA;
	result.m[1][2] = sinA;
	result.m[2][1] = -sinA;
	result.m[2][2] = cosA;
	return result;
}

Matrix4x4 rotateY(float angle)
{
	Matrix4x4 result = MakeIdentityMatrix4x4();
	float cosA = std::cos(angle);
	float sinA = std::sin(angle);
	result.m[0][0] = cosA;
	result.m[0][2] = -sinA;
	result.m[2][0] = sinA;
	result.m[2][2] = cosA;
	return result;
}

Matrix4x4 rotateZ(float angle)
{
	Matrix4x4 result = MakeIdentityMatrix4x4();
	float cosA = std::cos(angle);
	float sinA = std::sin(angle);
	result.m[0][0] = cosA;
	result.m[0][1] = sinA;
	result.m[1][0] = -sinA;
	result.m[1][1] = cosA;
	return result;
}


Matrix4x4 MakeTranslationMatrix(const Vector3& translate)
{
	return
	{
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		translate.x, translate.y, translate.z, 1.0f
	};

}

Matrix4x4 MakeScaleMatrix(const Vector3& scale)
{
	return
	{
		scale.x, 0.0f, 0.0f, 0.0f,
		0.0f, scale.y, 0.0f, 0.0f,
		0.0f, 0.0f, scale.z, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};
}


Matrix4x4 makeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate)
{

	Matrix4x4 scaleMatrix = MakeScaleMatrix(scale);

	Matrix4x4 rotateXMatrix = rotateX(rotate.x);
	Matrix4x4 rotateYMatrix = rotateY(rotate.y);
	Matrix4x4 rotateZMatrix = rotateZ(rotate.z);

	Matrix4x4 rotateMatrix = Multiply(rotateXMatrix, Multiply(rotateYMatrix, rotateZMatrix));

	Matrix4x4 translationMatrix = MakeTranslationMatrix(translate);

	Matrix4x4 result = Multiply(scaleMatrix, Multiply(rotateMatrix, translationMatrix));

	return result;
}

Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip)
{
	Matrix4x4 result = { 0 };
	float tanHalfFovY = std::tan(fovY / 2.0f);

	result.m[0][0] = 1.0f / (aspectRatio * tanHalfFovY);
	result.m[1][1] = 1.0f / tanHalfFovY;
	result.m[2][2] = farClip / (farClip - nearClip);
	result.m[2][3] = 1.0f;
	result.m[3][2] = (-nearClip * farClip) / (farClip - nearClip);

	return result;
}

Matrix4x4 Inverse(const Matrix4x4& m)
{
	float mat[4][8];
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			mat[i][j] = m.m[i][j];
			mat[i][j + 4] = (i == j) ? 1.0f : 0.0f;
		}
	}

	for (int i = 0; i < 4; ++i)
	{
		int pivot = i;
		for (int j = i + 1; j < 4; ++j)
		{
			if (std::abs(mat[j][i]) > std::abs(mat[pivot][i])) {
				pivot = j;
			}
		}

		for (int j = 0; j < 8; ++j)
		{
			std::swap(mat[i][j], mat[pivot][j]);
		}

		if (std::abs(mat[i][i]) < 1e-6f)
		{
			return MakeIdentityMatrix4x4();
		}

		float factor = mat[i][i];
		for (int j = 0; j < 8; ++j)
		{
			mat[i][j] /= factor;
		}

		for (int k = 0; k < 4; ++k)
		{
			if (k != i)
			{
				float subFactor = mat[k][i];
				for (int j = 0; j < 8; ++j)
				{
					mat[k][j] -= subFactor * mat[i][j];
				}
			}
		}
	}

	Matrix4x4 result;
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			result.m[i][j] = mat[i][j + 4];
		}
	}
	return result;
}


std::string ConvertString(const std::wstring& str)
{
	if (str.empty())
	{
		return std::string();
	}

	auto sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);
	if (sizeNeeded == 0)
	{
		return std::string();
	}
	std::string result(sizeNeeded, 0);
	WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), sizeNeeded, NULL, NULL);
	return result;
}

//std::wstring ConvertString(const std::string& str) 
//{
//	if (str.empty()) 
//	{
//		return std::wstring();
//	}
//
//	auto sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), NULL, 0);
//	if (sizeNeeded == 0) 
//	{
//		return std::wstring();
//	}
//	std::wstring result(sizeNeeded, 0);
//	MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), &result[0], sizeNeeded);
//	return result;
//}

void Log(const std::string& message)
{
	OutputDebugStringA(message.c_str());
}

IDxcBlob* CompileShader(
	//CompilerするShaderへのファイルへのパス
	const std::wstring& filePath,
	//Compilerに使用する
	const wchar_t* profile,
	//初期化で生成したものを３つ
	IDxcUtils* dxcUtils,
	IDxcCompiler3* dxcCompiler,
	IDxcIncludeHandler* includeHandler)
{
	//1.HLSLファイルを読む
	Log(ConvertString(std::format(L"Begin CompilerShader, path:{}, profile:{}\n", filePath, profile)));
	IDxcBlobEncoding* shaderSource = nullptr;
	HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);
	assert(SUCCEEDED(hr));

	DxcBuffer shaderSourceBuffer;
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
	shaderSourceBuffer.Encoding = DXC_CP_UTF8;
	//2.Compileする
	LPCWSTR arguments[] =
	{
		filePath.c_str(),
		L"-E",L"main",
		L"-T",profile,
		L"-Zi",L"-Qembed_debug",
		L"-Od",
		L"-Zpr",
	};
	IDxcResult* shaderResult = nullptr;
	hr = dxcCompiler->Compile
	(
		&shaderSourceBuffer,
		arguments,
		_countof(arguments),
		includeHandler,
		IID_PPV_ARGS(&shaderResult)
	);
	assert(SUCCEEDED(hr));
	//３．警告・エラーが出てないか確認する
	IDxcBlobUtf8* shaderError = nullptr;
	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
	if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
		Log(shaderError->GetStringPointer());
		assert(false);
	}
	// 4.Compile結果を受け取って返す
	IDxcBlob* shaderBlob = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
	assert(SUCCEEDED(hr));
	Log(ConvertString(std::format(L"Compile Succeeded, path{},profile:{}\n", filePath, profile)));
	shaderSource->Release();
	shaderResult->Release();
	return shaderBlob;
}

//ウィンドウプロシージャ
LRESULT CALLBACK  WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
	{
		return true;
	}


	//メッセージに応じてゲーム固有の処理を行う
	switch (msg)
	{
		//ウィンドウが破棄された
	case WM_DESTROY:
		//OSに対して、アプリの終了を伝える
		PostQuitMessage(0);
		return 0;
	}

	//標準のメッセージ処理を行う
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

ID3D12DescriptorHeap* CreateDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible)

{
	ID3D12DescriptorHeap* descriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.Type = heapType;
	descriptorHeapDesc.NumDescriptors = numDescriptors;
	descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HRESULT hr = device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
	assert(SUCCEEDED(hr));
	return descriptorHeap;
}

ID3D12Resource* CreateBufferResource(ID3D12Device* device, size_t sizeInBytes)
{
	// 頂点リソース用のヒープの設定
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

	// リソースの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = sizeInBytes; // 引数で受け取ったサイズ
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ID3D12Resource* resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&uploadHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&resource)
	);
	assert(SUCCEEDED(hr));

	return resource;
}

//Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
{

#ifdef _DEBUG

	ID3D12Debug1* debugController = nullptr;

	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		//デバッグレイヤーを有効にする
		debugController->EnableDebugLayer();

		//さらにGPU側でもチェックを行うようにする
		debugController->SetEnableSynchronizedCommandQueueValidation(true);
	}

#endif // _DEBUG

	//DXGIファクトリーの生成
	IDXGIFactory7* dxiFactory = nullptr;

	HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxiFactory));

	assert(SUCCEEDED(hr));

	//使用するアダプタ用の変数。最初にnullptrを入れておく
	IDXGIAdapter4* useAdapter = nullptr;

	//良い順にアダプタを頼む
	for (UINT i = 0; dxiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND; i++)
	{
		//アダプタの情報を取得する
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr));//取得できないのは一大事
		//ソフトウェアアダプタでなければ採用！
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE))
		{
			//採用したアダプタの情報をログに出力。wstringの方なので注意
			Log(ConvertString(std::format(L"Use Adapter:{}\n", adapterDesc.Description)));
			break;
		}
		useAdapter = nullptr;//ソフトウェアアダプタの場合は見なかったことにする
	}
	//適切なアダプタが見つからなかったので起動できない
	assert(useAdapter != nullptr);

	ID3D12Device* device = nullptr;

	//機能レベルとログ出力用の文字列
	D3D_FEATURE_LEVEL featureLevels[] =
	{
	   D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0,
	};

	const char* featureLevelStrings[] =
	{
		"12.2", "12.1", "12.0",
	};

	//高い順に生成できるか試していく
	for (size_t i = 0; i < _countof(featureLevels); i++)
	{
		//採用したアダプターでデバイスを生成
		hr = D3D12CreateDevice(useAdapter, featureLevels[i], IID_PPV_ARGS(&device));
		//指定した機能レベルで生成できたかを確認
		if (SUCCEEDED(hr))
		{
			//生成できた場合はログ出力行ってループを抜ける
			Log(std::format("FeatureLevel:{}\n", featureLevelStrings[i]));
			break;
		}
	}

	//デバイスの生成がうまくいかなかった場合は起動できない
	assert(device != nullptr);
	Log("Complete create D3D12Device\n"); // 初期化完了のログを出す

#ifdef _DEBUG

	ID3D12InfoQueue* infoQueue = nullptr;
	if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue))))
	{
		//やばいエラー時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);

		//エラー時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);

		//警告時に止まる
		//infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

		//抑制するメッセージのID
		D3D12_MESSAGE_ID denyIds[] =
		{
			//Windows11でのDXGIデバックレイヤーとDX１２のデバックレイヤーの相互作用バグによるエラーメッセージ
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
		};

		//抑制するレベル
		D3D12_MESSAGE_SEVERITY severities[] =
		{
			D3D12_MESSAGE_SEVERITY_INFO
		};

		D3D12_INFO_QUEUE_FILTER filter{};

		filter.DenyList.NumIDs = _countof(denyIds);
		filter.DenyList.pIDList = denyIds;
		filter.DenyList.NumSeverities = _countof(severities);
		filter.DenyList.pSeverityList = severities;

		//指定したメッセージの表示を抑制する
		infoQueue->PushStorageFilter(&filter);

		//解放
		infoQueue->Release();
	}

#endif // _DEBUG


	WNDCLASS wc{};
	//ウィンドウプロシージャ
	wc.lpfnWndProc = WindowProc;

	//ウィンドウクラス名
	wc.lpszClassName = L"CG2WindowClass";

	//インスタンスハンドル
	wc.hInstance = GetModuleHandle(nullptr);

	//カーソル
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

	//ウィンドウクラスの登録
	RegisterClass(&wc);

	//クライアント領域のサイズ
	const int32_t kClientWidth = 1280;
	const int32_t kClientHeight = 720;

	//ウィンドウサイズを表す構造体にクライアント領域を入れる
	RECT wrc = { 0,0,kClientWidth,kClientHeight };
	//クライアント領域を元に実際のサイズにwrcを変更してもらう
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	//ウィンドウの生成
	HWND hwnd = CreateWindow(
		wc.lpszClassName, //利用するクラス名
		L"CG2", //タイトルバーの文字
		WS_OVERLAPPEDWINDOW,//よく見るウィンドウスタイル
		CW_USEDEFAULT,//表示X座標（OSに任せる）
		CW_USEDEFAULT,//表示Y座標（OSに任せる）
		wrc.right - wrc.left,//ウィンドウ横幅
		wrc.bottom - wrc.top,//ウィンドウ縦幅
		nullptr,//親ウィンドウハンドル
		nullptr,//メニューハンドル
		wc.hInstance,//インスタンスハンドル
		nullptr//オプション
	);

	ShowWindow(hwnd, SW_SHOW);

	ID3D12CommandQueue* commandQueue = nullptr;
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	hr = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue));
	//コマンドキューの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));

	//スワップチェーンを生成する
	IDXGISwapChain4* swapChain = nullptr;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = kClientWidth;//画面の幅、ウィンドウのクライアント領域の幅と同じにする
	swapChainDesc.Height = kClientHeight;//画面の高さ、ウィンドウのクライアント領域の高さと同じにする
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;//色の形式
	swapChainDesc.SampleDesc.Count = 1;//マルチサンプルしない
	swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;//描画のターゲットとして利用する
	swapChainDesc.BufferCount = 2;//ダブルバッファ
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;//モニタをうつしたら、中身を破棄
	//コマンドキュー、ウィンドウハンドル、設定を渡して生成する
	hr = dxiFactory->CreateSwapChainForHwnd(commandQueue, hwnd, &swapChainDesc, nullptr, nullptr, (IDXGISwapChain1**)&swapChain);
	assert(SUCCEEDED(hr));

	ID3D12CommandAllocator* commandAllocator = nullptr;
	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	//コマンドアロケーターの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));

	//コマンドリストの生成
	ID3D12GraphicsCommandList* commandList = nullptr;
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, IID_PPV_ARGS(&commandList));
	//コマンドリストの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));

	//ディスクリプタヒープの生成
	ID3D12DescriptorHeap* trvDescriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC trvDescriptorHeapDesc{};
	trvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;//レンダーターゲットビュー用
	trvDescriptorHeapDesc.NumDescriptors = 2;//ダブルバッファリング
	hr = device->CreateDescriptorHeap(&trvDescriptorHeapDesc, IID_PPV_ARGS(&trvDescriptorHeap));
	//ディスクリプタビープの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));

	//SwapChainからResourceを引っ張ってくる
	ID3D12Resource* swapChainResources[2] = { nullptr };

	hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));

	//うまく取得できなかったので起動できない
	assert(SUCCEEDED(hr));
	hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));
	assert(SUCCEEDED(hr));

	//RTVの設定
	//D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	//rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;//フォーマットはスワップチェーンと同じ
	//rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;//2Dテクスチャとして書き込む
	////ディスクリプタの先頭を取得する
	//D3D12_CPU_DESCRIPTOR_HANDLE rtvStatHandle = trvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	////RTVを２つ作るのでディスクリプタを二つ用意
	//D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];
	////まず一つ目を作る。一つ目は最初のところにつくる。作る場所をこちらで指定してあげる必要がある
	//rtvHandles[0] = rtvStatHandle;
	//device->CreateRenderTargetView(swapChainResources[0], &rtvDesc, rtvHandles[0]);
	////二つ目のディスクリプタハンドルを得る。
	//rtvHandles[1].ptr = rtvHandles[0].ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	////二つ目を作る
	//device->CreateRenderTargetView(swapChainResources[1], &rtvDesc, rtvHandles[1]);
	if (trvDescriptorHeap)
	{
		trvDescriptorHeap->Release();
	}
	ID3D12DescriptorHeap* rtvDescriptorHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
	ID3D12DescriptorHeap* srvDescriptorHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];

	rtvHandles[0] = rtvStartHandle;
	device->CreateRenderTargetView(swapChainResources[0], &rtvDesc, rtvHandles[0]);

	rtvHandles[1].ptr = rtvHandles[0].ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	device->CreateRenderTargetView(swapChainResources[1], &rtvDesc, rtvHandles[1]);

	Log("Hello,DirectX!\n");

	MSG msg{};

	//初期値０でFenceを作る
	ID3D12Fence* fence = nullptr;
	uint64_t fenceValue = 0;
	hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	assert(SUCCEEDED(hr));

	//FenceのSignalを待つためのイベントを作成する
	HANDLE fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent != nullptr);

	//dxcCompilerを初期化
	IDxcUtils* dxcUtils = nullptr;
	IDxcCompiler3* dxcComplier = nullptr;
	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
	assert(SUCCEEDED(hr));
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcComplier));
	assert(SUCCEEDED(hr));

	//現時点でincludeはしないが、includeに対応するための設定をしておく
	IDxcIncludeHandler* includeHandler = nullptr;
	hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
	assert(SUCCEEDED(hr));
	//RootSignature作成
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	//D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	//RootSignature作成
	D3D12_ROOT_PARAMETER rootParameters[2] = {};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[0].Descriptor.ShaderRegister = 0;
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters[1].Descriptor.ShaderRegister = 0;
	descriptionRootSignature.NumParameters = _countof(rootParameters);
	descriptionRootSignature.pParameters = rootParameters;

	ID3D12Resource* wvpResource = CreateBufferResource(device, sizeof(Matrix4x4));

	Matrix4x4* wvpData = nullptr;

	wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));

	*wvpData = MakeIdentity4x4();

	//シリアライズしてバイナリにする
	ID3DBlob* signatureBlob = nullptr;
	ID3D10Blob* errorBlob = nullptr;

	//InputLayout
	D3D12_INPUT_ELEMENT_DESC inputElementDesc[1] = {};
	inputElementDesc[0].SemanticName = "POSITION";
	inputElementDesc[0].SemanticIndex = 0;
	inputElementDesc[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDesc[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDesc;
	inputLayoutDesc.NumElements = _countof(inputElementDesc);

	//BlendStateの設定
	D3D12_BLEND_DESC blendDesc{};
	//全ての色要素を書きこむ
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	//RasterizerStateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc{};

	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;

	//三角形の中を塗りつぶす
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	//Shaderをコンパイルする
	IDxcBlob* vertexShaderBlob = CompileShader(L"Object3d.VS.hlsl", L"vs_6_0", dxcUtils, dxcComplier, includeHandler);
	assert(vertexShaderBlob != nullptr);

	IDxcBlob* pixelShaderBlob = CompileShader(L"Object3d.PS.hlsl", L"ps_6_0", dxcUtils, dxcComplier, includeHandler);
	assert(pixelShaderBlob != nullptr);

	hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr))
	{
		Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}
	//バイナリを元に生成
	ID3D12RootSignature* rootSignature = nullptr;
	hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(hr));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicPipelineStateDesc{};

	graphicPipelineStateDesc.pRootSignature = rootSignature;

	graphicPipelineStateDesc.InputLayout = inputLayoutDesc;

	graphicPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(),vertexShaderBlob->GetBufferSize() };

	graphicPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(),pixelShaderBlob->GetBufferSize() };

	graphicPipelineStateDesc.BlendState = blendDesc;

	graphicPipelineStateDesc.RasterizerState = rasterizerDesc;

	//書きこむRTVの情報
	graphicPipelineStateDesc.NumRenderTargets = 1;
	graphicPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	//利用するリポジトリ（形状）のタイプ。三角形
	graphicPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	//どのように画面に色を打ち込むのか設定
	graphicPipelineStateDesc.SampleDesc.Count = 1;
	graphicPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	//実際に作成
	ID3D12PipelineState* graphicsPipelineState = nullptr;
	hr = device->CreateGraphicsPipelineState(&graphicPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));
	assert(SUCCEEDED(hr));

	//ID3D12Resource* vertexResource = CreateBufferResource(device, sizeof(Vector4) * 3);

	

	

	//頂点バッファビューを作成する
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	//リソースの先頭のアドレスから使う
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	//使用するリソースのサイズは頂点３つ分のサイズ
	vertexBufferView.SizeInBytes = sizeof(Vector4) * 3;
	//１頂点あたりのサイズ
	vertexBufferView.StrideInBytes = sizeof(Vector4);

	
	//マテリアルにデータを書き込む
	Vector4* materialData = nullptr;
	//書き込むためのアドレスを取得
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
    	
	Vector4 changeColor = { 1.0f,0.0f,0.0f,1.0f };

	//今回は赤を書き込んでみる
	*materialData = { 1.0f, 0.0f, 0.0f, 1.0f };

	*materialData = changeColor;

	//ビューポート
	D3D12_VIEWPORT viewport{};

	viewport.Width = kClientWidth;
	viewport.Height = kClientHeight;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	D3D12_RECT scissorRect{};
	scissorRect.left = 0;
	scissorRect.right = kClientWidth;
	scissorRect.top = 0;
	scissorRect.bottom = kClientHeight;



	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX12_Init
	(
		device,
		swapChainDesc.BufferCount,
		rtvDesc.Format,
		srvDescriptorHeap,
		srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart()
	);

	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->Build();

	Transform transform
	{
	 { 1.0f, 1.0f, 1.0f },
	 { 0.0f, 0.0f, 0.0f },
	 { 0.0f, 0.0f, -0.5f }
	};

	CameraTransform cameraTransform
	{
		{ 1.0f, 1.0f, 1.0f },
		{ 0.0f, 0.0f, 0.0f },
		{ 0.0f, 0.0f, -5.0f }
	};

	//ウィンドウのｘボタンが押されるまでループ
	while (msg.message != WM_QUIT)
	{
		//Windowsにメッセージが来てたら最優先で処理をさせる
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			//ゲームの処理
			UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			//ImGui::Begin("TRIANGLE_CHANGE_COLOR");

			ImGui::ShowDemoWindow();

			//ImGui::End();

			transform.rotate.y += 0.02f;

			Matrix4x4 projactonMatrix = MakePerspectiveFovMatrix(0.45f, static_cast<float>(kClientWidth) / kClientHeight, 0.1f, 100.f);
			Matrix4x4 worldMatrix =makeAffineMatrix(transform.scale,transform.rotate,transform.translate);
			Matrix4x4 cameraMatrix = makeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
			Matrix4x4 viewMatrix = Inverse(cameraMatrix);
			Matrix4x4 projctionMatrix = MakePerspectiveFovMatrix(0.45f, static_cast<float>(kClientWidth) / kClientHeight, 0.1f, 100.0f);
			Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrix, Multiply(viewMatrix, projctionMatrix));
			*wvpData = worldViewProjectionMatrix;
			

			*materialData = changeColor;

			//Imgui
			ImGui::Render();

			//TransitionBarrerの設定
			D3D12_RESOURCE_BARRIER barrier{};
			//今回のバリアはTransition
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			//Noneにしておく
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			//バリアをはる対象のリソース。現在のバックバッファに対して行う
			barrier.Transition.pResource = swapChainResources[backBufferIndex];
			//遷移前（現在）のResourceState
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
			//遷移後のResourceState
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
			//TransitionBarrerを張る
			commandList->ResourceBarrier(1, &barrier);

			//描画先のRTVを設定する
			commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, nullptr);

			////指定した色で画面全体をクリアする
			float clearColor[] = { 0.1f,0.25f,0.5f,1.0f };//青っぽい色、RGBAの順番
			commandList->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0, nullptr);

			//描画用のDescriptorHeap
			ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap };
			commandList->SetDescriptorHeaps(1, descriptorHeaps);


			commandList->RSSetViewports(1, &viewport);
			commandList->RSSetScissorRects(1, &scissorRect);
			commandList->SetGraphicsRootSignature(rootSignature);
			commandList->SetPipelineState(graphicsPipelineState);
			commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
			
			commandList->SetGraphicsRootConstantBufferView(1, wvpResource->GetGPUVirtualAddress());

			commandList->DrawInstanced(3, 1, 0, 0);

			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

			//今回はRenderTargetからpresentにする
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

			//TransitionBarrerを張る
			commandList->ResourceBarrier(1, &barrier);

			//コマンドリストの内容を確定させる、全てのコマンドを積んでからCloseする必要がある
			hr = commandList->Close();
			assert(SUCCEEDED(hr));

			//GPUにコマンドリストの実行を行わせる
			ID3D12CommandList* commandLists[] = { commandList };
			commandQueue->ExecuteCommandLists(1, commandLists);

			//GPUとOSに画面の交換を行うように指示する
			swapChain->Present(1, 0);

			//Fenceの値を更新
			fenceValue++;
			//GPUがここまでたどり着いたときに、Fenceの値をしてした値に代入するようにsignalを送る
			commandQueue->Signal(fence, fenceValue);

			//Fenceの値が指定したsignalにたどり着いているか確認する
			//GetCompleteValueの初期値はFence作成時にわたした初期値
			if (fence->GetCompletedValue() < fenceValue)
			{
				// 指定した値になるまで待つようにイベントを設定
				fence->SetEventOnCompletion(fenceValue, fenceEvent);
				// イベントが発生するまでCPUを停止させる
				WaitForSingleObject(fenceEvent, INFINITE);
			}
			hr = commandAllocator->Reset();
			assert(SUCCEEDED(hr));

			hr = commandList->Reset(commandAllocator, nullptr);
			assert(SUCCEEDED(hr));
		}
	}

	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	if (includeHandler)
	{
		includeHandler->Release();
	}

	if (dxcComplier)
	{
		dxcComplier->Release();
	}

	if (dxcUtils)
	{
		dxcUtils->Release();
	}

	CloseHandle(fenceEvent);

	if (fence)
	{
		fence->Release();
	}

	
	if (rtvDescriptorHeap)
	{
		rtvDescriptorHeap->Release();
	}

	if (srvDescriptorHeap)
	{
		srvDescriptorHeap->Release();
	}

	if (swapChainResources[0])
	{
		swapChainResources[0]->Release();
	}

	if (swapChainResources[1])
	{
		swapChainResources[1]->Release();
	}

	if (swapChain)
	{
		swapChain->Release();
	}

	if (commandList)
	{
		commandList->Release();
	}

	if (commandAllocator)
	{
		commandAllocator->Release();
	}

	if (commandQueue)
	{
		commandQueue->Release();
	}

	if (vertexResource)
	{
		vertexResource->Release();
	}

	if (materialResource)
	{
		materialResource->Release();
	}

	if (graphicsPipelineState)
	{
		graphicsPipelineState->Release();
	}

	if (errorBlob)
	{
		errorBlob->Release();
	}

	if (rootSignature)
	{
		rootSignature->Release();
	}

	if (pixelShaderBlob)
	{
		pixelShaderBlob->Release();
	}

	if (vertexShaderBlob)
	{
		vertexShaderBlob->Release();
	}

	if (device)
	{
		device->Release();
	}

	if (useAdapter)
	{
		useAdapter->Release();
	}

	if (dxiFactory)
	{
		dxiFactory->Release();
	}

#ifdef _DEBUG
	if (debugController)
	{
		debugController->Release();
	}
#endif // _DEBUG

	if (hwnd)
	{
		DestroyWindow(hwnd);
	}

	// リソースリークチェック
	IDXGIDebug1* debug;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug))))
	{
		debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
		debug->Release();
	}

	return 0;
}

