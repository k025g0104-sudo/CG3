#pragma once
#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxcapi.h>
#include <cstdint>
#include "WinApp.h"

class DirectXCommon
{
public:
	void Initialize(WinApp* winApp);
	void PreDraw();
	void PostDraw();
	void Finalize();


	ID3D12Device* GetDevice() const { return device_.Get(); }
	ID3D12GraphicsCommandList* GetCommandList() const { return commandList_.Get(); }
	ID3D12DescriptorHeap* GetSrvDescriptorHeap() const { return srvDescriptorHeap_.Get(); }
	IDxcUtils* GetDxcUtils() const { return dxcUtils_.Get(); }
	IDxcCompiler3* GetDxcCompiler() const { return dxcCompiler_.Get(); }
	IDxcIncludeHandler* GetIncludeHandler() const { return includeHandler_.Get(); }

private:

	void InitializeDevice();
	void InitializeCommand();
	void CreateSwapChain();
	void CreateDepthBuffer();
	void CreateDescriptorHeaps();
	void InitializeRenderTargetView();
	void InitializeDepthStencilView();
	void CreateFence();
	void InitializeViewport();
	void InitializeScissorRect();
	void CreateDXCCompiler();
	void InitializeImGui();

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(
		D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(int32_t width, int32_t height);

private:
	static constexpr UINT kBackBufferCount = 2;
	static constexpr DXGI_FORMAT kRtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	static constexpr DXGI_FORMAT kDsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	WinApp* winApp_ = nullptr;

	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_;
	Microsoft::WRL::ComPtr<ID3D12Device> device_;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator_;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_;

	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_;

	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource_;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap_;
	Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResources_[kBackBufferCount];
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles_[kBackBufferCount]{};

	Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
	uint64_t fenceValue_ = 0;
	HANDLE fenceEvent_ = nullptr;

	D3D12_VIEWPORT viewport_{};
	D3D12_RECT scissorRect_{};

	Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils_;
	Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler_;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler_;
};