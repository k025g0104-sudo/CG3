#include "DirectXCommon.h"
#include <cassert>
#include <format>
#include "Logger.h"
#include "StringUtility.h"
#ifdef _DEBUG
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_win32.h"
#include "externals/imgui/imgui_impl_dx12.h"
#endif // _DEBUG

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"dxcompiler.lib")

using namespace Microsoft::WRL;

void DirectXCommon::Initialize(WinApp* winApp)
{

}

void DirectXCommon::InitializeDevice()
{
#ifdef _DEBUG
	ComPtr<ID3D12Debug1> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		// デバッグレイヤーを有効にする
		debugController->EnableDebugLayer();
		// さらにGPU側でもチェックを行うようにする
		debugController->SetEnableSynchronizedCommandQueueValidation(true);
	}
#endif // _DEBUG

	// DXGIファクトリーの生成
	HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory_));
	assert(SUCCEEDED(hr));

	// 使用するアダプタ
	ComPtr<IDXGIAdapter4> useAdapter;

	// 良い順にアダプタを頼む
	for (UINT i = 0; dxgiFactory_->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND; i++)
	{
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr));
		// ソフトウェアアダプタでなければ採用
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE))
		{
			Log(ConvertString(std::format(L"Use Adapter:{}\n", adapterDesc.Description)));
			break;
		}
		useAdapter.Reset(); // ソフトウェアアダプタは見なかったことにする
	}
	assert(useAdapter != nullptr);

	// 機能レベルとログ出力用の文字列
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0,
	};
	const char* featureLevelStrings[] = { "12.2", "12.1", "12.0" };

	// 高い順に生成できるか試していく
	for (size_t i = 0; i < _countof(featureLevels); i++)
	{
		hr = D3D12CreateDevice(useAdapter.Get(), featureLevels[i], IID_PPV_ARGS(&device_));
		if (SUCCEEDED(hr))
		{
			Log(std::format("FeatureLevel:{}\n", featureLevelStrings[i]));
			break;
		}
	}
	assert(device_ != nullptr);
	Log("Complete create D3D12Device\n");

#ifdef _DEBUG
	ComPtr<ID3D12InfoQueue> infoQueue;
	if (SUCCEEDED(device_->QueryInterface(IID_PPV_ARGS(&infoQueue))))
	{
		// やばいエラー時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		// エラー時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);

		// 抑制するメッセージのID(Windows11でのDXGIとDX12のデバッグレイヤーの相互作用バグ)
		D3D12_MESSAGE_ID denyIds[] =
		{
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
		};
		// 抑制するレベル
		D3D12_MESSAGE_SEVERITY severities[] =
		{
			D3D12_MESSAGE_SEVERITY_INFO
		};

		D3D12_INFO_QUEUE_FILTER filter{};
		filter.DenyList.NumIDs = _countof(denyIds);
		filter.DenyList.pIDList = denyIds;
		filter.DenyList.NumSeverities = _countof(severities);
		filter.DenyList.pSeverityList = severities;

		// 指定したメッセージの表示を抑制する
		infoQueue->PushStorageFilter(&filter);
	}
#endif // _DEBUG
}

// ② コマンド関連の生成
void DirectXCommon::InitializeCommand()
{
	// コマンドキュー
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	HRESULT hr = device_->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue_));
	assert(SUCCEEDED(hr));

	// コマンドアロケーター
	hr = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator_));
	assert(SUCCEEDED(hr));

	// コマンドリスト
	hr = device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator_.Get(), nullptr, IID_PPV_ARGS(&commandList_));
	assert(SUCCEEDED(hr));
}

// ③ スワップチェーンの生成
void DirectXCommon::CreateSwapChain()
{
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = WinApp::kClientWidth;
	swapChainDesc.Height = WinApp::kClientHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapChainDesc.BufferCount = kBackBufferCount;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	ComPtr<IDXGISwapChain1> swapChain1;
	HRESULT hr = dxgiFactory_->CreateSwapChainForHwnd(
		commandQueue_.Get(), winApp_->GetHwnd(), &swapChainDesc, nullptr, nullptr, &swapChain1);
	assert(SUCCEEDED(hr));

	// IDXGISwapChain4 として受け取る
	hr = swapChain1.As(&swapChain_);
	assert(SUCCEEDED(hr));
}

// ④ 深度バッファの生成
void DirectXCommon::CreateDepthBuffer()
{
	depthStencilResource_ = CreateDepthStencilTextureResource(WinApp::kClientWidth, WinApp::kClientHeight);
}

void DirectXCommon::CreateDescriptorHeaps()
{
	rtvDescriptorHeap_ = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, kBackBufferCount, false);
	srvDescriptorHeap_ = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);
	dsvDescriptorHeap_ = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);
}

void DirectXCommon::InitializeRenderTargetView()
{
	// SwapChainからResourceを引っ張ってくる
	for (UINT i = 0; i < kBackBufferCount; ++i)
	{
		HRESULT hr = swapChain_->GetBuffer(i, IID_PPV_ARGS(&swapChainResources_[i]));
		assert(SUCCEEDED(hr));
	}

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = kRtvFormat;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	const UINT rtvSize = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();

	for (UINT i = 0; i < kBackBufferCount; ++i)
	{
		rtvHandles_[i].ptr = rtvStartHandle.ptr + static_cast<SIZE_T>(rtvSize) * i;
		device_->CreateRenderTargetView(swapChainResources_[i].Get(), &rtvDesc, rtvHandles_[i]);
	}
}

void DirectXCommon::InitializeDepthStencilView()
{
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = kDsvFormat;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	device_->CreateDepthStencilView(depthStencilResource_.Get(), &dsvDesc, dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart());
}

void DirectXCommon::CreateFence()
{
	fenceValue_ = 0;
	HRESULT hr = device_->CreateFence(fenceValue_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
	assert(SUCCEEDED(hr));

	// FenceのSignalを待つためのイベントを作成する
	fenceEvent_ = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent_ != nullptr);
}

void DirectXCommon::InitializeViewport()
{
	viewport_.Width = WinApp::kClientWidth;
	viewport_.Height = WinApp::kClientHeight;
	viewport_.TopLeftX = 0;
	viewport_.TopLeftY = 0;
	viewport_.MinDepth = 0.0f;
	viewport_.MaxDepth = 1.0f;
}

void DirectXCommon::InitializeScissorRect()
{
	scissorRect_.left = 0;
	scissorRect_.right = WinApp::kClientWidth;
	scissorRect_.top = 0;
	scissorRect_.bottom = WinApp::kClientHeight;
}

void DirectXCommon::CreateDXCCompiler()
{
	HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils_));
	assert(SUCCEEDED(hr));
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler_));
	assert(SUCCEEDED(hr));

	// includeに対応するための設定
	hr = dxcUtils_->CreateDefaultIncludeHandler(&includeHandler_);
	assert(SUCCEEDED(hr));
}

// ⑫ ImGuiの初期化
void DirectXCommon::InitializeImGui()
{
#ifdef _DEBUG
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(winApp_->GetHwnd());
	ImGui_ImplDX12_Init(
		device_.Get(),
		kBackBufferCount,
		kRtvFormat,
		srvDescriptorHeap_.Get(),
		srvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart(),
		srvDescriptorHeap_->GetGPUDescriptorHandleForHeapStart());

	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->Build();
#endif // _DEBUG
}

void DirectXCommon::PreDraw()
{
	UINT backBufferIndex = swapChain_->GetCurrentBackBufferIndex();

	// Present -> RenderTarget
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = swapChainResources_[backBufferIndex].Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	commandList_->ResourceBarrier(1, &barrier);

	// 描画先のRTVとDSVを設定する
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
	commandList_->OMSetRenderTargets(1, &rtvHandles_[backBufferIndex], false, &dsvHandle);

	// 画面全体をクリアする
	float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
	commandList_->ClearRenderTargetView(rtvHandles_[backBufferIndex], clearColor, 0, nullptr);
	commandList_->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// 描画用のDescriptorHeap
	ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap_.Get() };
	commandList_->SetDescriptorHeaps(1, descriptorHeaps);

	commandList_->RSSetViewports(1, &viewport_);
	commandList_->RSSetScissorRects(1, &scissorRect_);
}

void DirectXCommon::PostDraw()
{
#ifdef _DEBUG
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList_.Get());
#endif // _DEBUG

	UINT backBufferIndex = swapChain_->GetCurrentBackBufferIndex();

	// RenderTarget -> Present
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = swapChainResources_[backBufferIndex].Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	commandList_->ResourceBarrier(1, &barrier);

	// コマンドリストの内容を確定させる
	HRESULT hr = commandList_->Close();
	assert(SUCCEEDED(hr));

	// GPUにコマンドリストの実行を行わせる
	ID3D12CommandList* commandLists[] = { commandList_.Get() };
	commandQueue_->ExecuteCommandLists(1, commandLists);

	// 画面の交換
	swapChain_->Present(1, 0);

	// Fenceの値を更新し、GPUが到達したらこの値になるようSignalを送る
	fenceValue_++;
	commandQueue_->Signal(fence_.Get(), fenceValue_);

	// GPUがまだ到達していなければ待つ
	if (fence_->GetCompletedValue() < fenceValue_)
	{
		fence_->SetEventOnCompletion(fenceValue_, fenceEvent_);
		WaitForSingleObject(fenceEvent_, INFINITE);
	}

	// 次のフレームのためにコマンドをリセット
	hr = commandAllocator_->Reset();
	assert(SUCCEEDED(hr));
	hr = commandList_->Reset(commandAllocator_.Get(), nullptr);
	assert(SUCCEEDED(hr));
}

void DirectXCommon::Finalize()
{
#ifdef _DEBUG
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
#endif // _DEBUG

	if (fenceEvent_)
	{
		CloseHandle(fenceEvent_);
		fenceEvent_ = nullptr;
	}
}

ComPtr<ID3D12DescriptorHeap> DirectXCommon::CreateDescriptorHeap(
	D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible)
{
	ComPtr<ID3D12DescriptorHeap> descriptorHeap;
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.Type = heapType;
	descriptorHeapDesc.NumDescriptors = numDescriptors;
	descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HRESULT hr = device_->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
	assert(SUCCEEDED(hr));
	return descriptorHeap;
}

ComPtr<ID3D12Resource> DirectXCommon::CreateDepthStencilTextureResource(int32_t width, int32_t height)
{
	// 生成するResourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = width;
	resourceDesc.Height = height;
	resourceDesc.MipLevels = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Format = kDsvFormat;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	// 利用するHeapの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	// 深度のクリア設定
	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f;
	depthClearValue.Format = kDsvFormat;

	ComPtr<ID3D12Resource> resource;
	HRESULT hr = device_->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthClearValue,
		IID_PPV_ARGS(&resource));
	assert(SUCCEEDED(hr));
	return resource;
}