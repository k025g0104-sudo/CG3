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
	// DirectX の初期化(スライドの12項目をこの順に呼ぶ)
	void Initialize(WinApp* winApp);

	// 描画前の処理(バリア・クリア・ビューポート設定など)
	void PreDraw();
	// 描画後の処理(ImGui描画・Present・GPU待ち・コマンドリセット)
	void PostDraw();
	// 終了処理(ImGui終了・イベントハンドル解放)
	void Finalize();

	ID3D12Device* GetDevice() const { return device_.Get(); }
	ID3D12GraphicsCommandList* GetCommandList() const { return commandList_.Get(); }
	ID3D12DescriptorHeap* GetSrvDescriptorHeap() const { return srvDescriptorHeap_.Get(); }
	IDxcUtils* GetDxcUtils() const { return dxcUtils_.Get(); }
	IDxcCompiler3* GetDxcCompiler() const { return dxcCompiler_.Get(); }
	IDxcIncludeHandler* GetIncludeHandler() const { return includeHandler_.Get(); }

private:
	// ---- Initialize から呼ぶ関数(スライドの項目順) ----
	void InitializeDevice();             // ① デバイス
	void InitializeCommand();            // ② コマンド関連
	void CreateSwapChain();              // ③ スワップチェーン
	void CreateDepthBuffer();            // ④ 深度バッファ
	void CreateDescriptorHeaps();        // ⑤ デスクリプタヒープ(RTV/SRV/DSV)
	void InitializeRenderTargetView();   // ⑥ RTV
	void InitializeDepthStencilView();   // ⑦ DSV
	void CreateFence();                  // ⑧ フェンス
	void InitializeViewport();           // ⑨ ビューポート
	void InitializeScissorRect();        // ⑩ シザリング矩形
	void CreateDXCCompiler();            // ⑪ DXCコンパイラ
	void InitializeImGui();              // ⑫ ImGui

	// ---- 内部だけで使う補助関数 ----
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(
		D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(int32_t width, int32_t height);

private:
	static constexpr UINT kBackBufferCount = 2;
	static constexpr DXGI_FORMAT kRtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	static constexpr DXGI_FORMAT kDsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	WinApp* winApp_ = nullptr;

	// ① デバイス
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_;
	Microsoft::WRL::ComPtr<ID3D12Device> device_;

	// ② コマンド
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator_;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_;

	// ③ スワップチェーン
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_;

	// ④ 深度バッファ
	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource_;

	// ⑤ ヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap_;

	// ⑥ RTV
	Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResources_[kBackBufferCount];
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles_[kBackBufferCount]{};

	// ⑧ フェンス
	Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
	uint64_t fenceValue_ = 0;
	HANDLE fenceEvent_ = nullptr;

	// ⑨ ⑩
	D3D12_VIEWPORT viewport_{};
	D3D12_RECT scissorRect_{};

	// ⑪ DXC
	Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils_;
	Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler_;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler_;
};