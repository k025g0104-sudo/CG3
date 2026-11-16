#pragma once
#include <Windows.h>
#include <cstdint>
#include <string>
#include <array>
#include <chrono>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxcapi.h>
#include <wrl.h>
#include "externals/DirectXTex/DirectXTex.h"
#include "WinApp.h"

// DirectX 基盤クラス
// デバイス〜ImGui の初期化、描画前後処理(PreDraw/PostDraw)、
// シェーダーコンパイル・バッファ/テクスチャ生成の補助関数をまとめる
class DirectXCommon
{
public:
	template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

	// ===== 基本の流れ =====
	void Initialize(WinApp* winApp);
	void Finalize();

	// 描画前処理(バリア、RTV/DSVクリア、ビューポート等、ImGuiフレーム開始)
	void PreDraw();
	// 描画後処理(ImGui描画、バリア、コマンド実行、Present、フェンス待ち)
	void PostDraw();

	// ===== ゲッター =====
	ID3D12Device* GetDevice() const { return device_.Get(); }
	ID3D12GraphicsCommandList* GetCommandList() const { return commandList_.Get(); }
	ID3D12DescriptorHeap* GetSrvDescriptorHeap() const { return srvDescriptorHeap_.Get(); }
	// ※ DXC関連(dxcUtils_ / dxcCompiler_ / includeHandler_)のゲッターは
	//    CompileShader をクラス内に移したため廃止

	// ===== DescriptorHeap のハンドル取得(index だけで位置が決まる) =====
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUDescriptorHandle(uint32_t index) const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUDescriptorHandle(uint32_t index) const;

	// ===== 補助関数群 =====
	// シェーダーのコンパイル
	IDxcBlob* CompileShader(const std::wstring& filePath, const wchar_t* profile);

	// バッファリソース(Upload Heap)の生成
	ComPtr<ID3D12Resource> CreateBufferResource(size_t sizeInBytes);

	// テクスチャリソースの生成
	ComPtr<ID3D12Resource> CreateTextureResource(const DirectX::TexMetadata& metadata);

	// テクスチャデータの転送
	void UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages);

	// テクスチャファイルの読み込み(メンバ変数に依存しないので static)
	static DirectX::ScratchImage LoadTexture(const std::string& filePath);

private:
	// ===== 初期化の内部処理 =====
	void InitializeDevice();
	void InitializeCommand();
	void InitializeSwapChain();
	void InitializeDepthBuffer();
	void InitializeDescriptorHeaps();
	void InitializeRenderTargetView();
	void InitializeDepthStencilView();
	void InitializeFence();
	void InitializeViewport();
	void InitializeScissorRect();
	void InitializeDxcCompiler();
	void InitializeImGui();

	// 汎用のDescriptorHeap生成
	ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(
		D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);

	// 汎用のDescriptorハンドル取得
	static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(
		ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index);
	static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(
		ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index);

private:
	// ウィンドウ(借りるだけ。所有しない)
	WinApp* winApp_ = nullptr;

	// DXGI / D3D12
	ComPtr<IDXGIFactory7> dxgiFactory_;
	ComPtr<ID3D12Device> device_;

	// コマンド関連
	ComPtr<ID3D12CommandQueue> commandQueue_;
	ComPtr<ID3D12CommandAllocator> commandAllocator_;
	ComPtr<ID3D12GraphicsCommandList> commandList_;

	// スワップチェーン
	ComPtr<IDXGISwapChain4> swapChain_;
	std::array<ComPtr<ID3D12Resource>, 2> swapChainResources_;

	// 深度バッファ
	ComPtr<ID3D12Resource> depthStencilResource_;

	// DescriptorHeap
	ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap_;
	ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap_;
	ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap_;
	uint32_t descriptorSizeRTV_ = 0;
	uint32_t descriptorSizeSRV_ = 0;
	uint32_t descriptorSizeDSV_ = 0;

	// RTV
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles_[2]{};

	// フェンス
	ComPtr<ID3D12Fence> fence_;
	uint64_t fenceValue_ = 0;
	HANDLE fenceEvent_ = nullptr;

	// ビューポート・シザー矩形
	D3D12_VIEWPORT viewport_{};
	D3D12_RECT scissorRect_{};

	// DXC(シェーダーコンパイラ)
	ComPtr<IDxcUtils> dxcUtils_;
	ComPtr<IDxcCompiler3> dxcCompiler_;
	ComPtr<IDxcIncludeHandler> includeHandler_;

	// バリア(PreDraw/PostDraw で共有)
	D3D12_RESOURCE_BARRIER barrier_{};
};