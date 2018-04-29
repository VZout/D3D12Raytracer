#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <array>

#include "viewer.hpp"
#include "vec.hpp"
#include "d3dx12.hpp"

using Microsoft::WRL::ComPtr;

class D3D12Viewer : public Viewer
{
	friend struct FenceObject;
public:
	explicit D3D12Viewer(Application& app);
	~D3D12Viewer() override;

	void NewFrame() override;
	void Present() override;

	static const D3D_FEATURE_LEVEL m_feature_level = D3D_FEATURE_LEVEL_12_1;
	static const DXGI_FORMAT m_back_buffer_format = DXGI_FORMAT_R8G8B8A8_UNORM;
	static const bool disable_gpu_timeout = false;
	static const std::uint8_t num_back_buffers = 2;
	static const bool use_vsync = false;
	const float m_clear_color[4] = { 0, 0, 0, 255 };

private:
	ComPtr<IDXGIFactory5> m_factory;
	ComPtr<ID3D12Device3> m_device;
	ComPtr<IDXGIAdapter4> m_adapter;
	ComPtr<IDXGISwapChain4> m_swap_chain;
	ComPtr<ID3D12CommandQueue> m_cmd_queue;
	ComPtr<ID3D12DescriptorHeap> imgui_descriptor_heap;
	std::array<ComPtr<ID3D12Resource>, num_back_buffers> m_render_targets;
	ComPtr<ID3D12DescriptorHeap> m_render_target_view_heap;
#ifdef _DEBUG
	ComPtr<ID3D12Debug1> debug_controller;
#endif

	std::uint32_t rtv_increment_size;
	std::uint32_t dsv_increment_size;
	std::uint32_t cbv_srv_uav_increment_size;
	std::uint32_t sampler_increment_size;

	std::uint8_t m_frame_idx;

	ComPtr<ID3D12CommandAllocator>* m_cmd_allocators;
	ComPtr<ID3D12GraphicsCommandList2> m_cmd_list;

	std::pair<D3D12_VIEWPORT, D3D12_RECT> m_viewport;

	FenceObject* fence;

	void SetupD3D12();
	void SetupImGui();
	void SetupSwapChain(std::uint16_t width, std::uint16_t height);

	void CreateFactory();
	void FindCompatibleAdapter();
	void CreateDevice();

	template<unsigned int N>
	[[nodiscard]] ComPtr<ID3D12CommandAllocator>* CreateCommandAllocators();
	[[nodiscard]] ComPtr<ID3D12GraphicsCommandList2> CreateGraphicsCommandList(ComPtr<ID3D12CommandAllocator>& allocator);
	[[nodiscard]] ComPtr<ID3D12DescriptorHeap> CreateDepthStencilHeap(std::uint16_t num_buffers);
	[[nodiscard]] ComPtr<ID3D12Resource> CreateDepthStencilBuffer(D3D12_CPU_DESCRIPTOR_HANDLE desc_handle, fm::vec2 size);
	[[nodiscard]] ComPtr<ID3D12Resource> CreateDepthStencilBuffer(D3D12_CPU_DESCRIPTOR_HANDLE desc_handle, std::uint16_t width, std::uint16_t height);
	[[nodiscard]] ComPtr<ID3D12DescriptorHeap> CreateRenderTargetViewHeap(std::uint16_t num_buffers);
	template<const std::uint16_t N>
	[[nodiscard]] std::array<ComPtr<ID3D12Resource>, N> GetRenderTargetsFromSwapChain(ComPtr<IDXGISwapChain4> swap_chain);
	template<const std::uint16_t N>
	void CreateRTVsFromResourceArray(std::array<ComPtr<ID3D12Resource>, N>& render_targets, CD3DX12_CPU_DESCRIPTOR_HANDLE desc_handle);
	[[nodiscard]] std::pair<D3D12_VIEWPORT, D3D12_RECT> CreateViewportAndRect(std::uint16_t width, std::uint16_t height);
};

struct FenceObject
{
	FenceObject(D3D12Viewer& viewer, const std::uint8_t num)
	{
		HRESULT hr;

		fences = new ComPtr<ID3D12Fence>[num];
		fence_values = new UINT64[num];

		// create the fences
		for (std::uint8_t i = 0; i < num; i++)
		{
			hr = viewer.m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fences[i]));
			if (FAILED(hr))
			{
				throw std::runtime_error("Failed to create fence.");
			}
			fence_values[i] = 0; // set the initial fence value to 0
		}

		// create a handle to a fence event
		fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (fence_event == nullptr)
		{
			throw std::runtime_error("Failed to create fence event.");
		}
	}

	inline void Increment(std::uint8_t idx)
	{
		fence_values[idx]++;
	}

	inline void Signal(ComPtr<ID3D12CommandQueue> queue, std::uint8_t idx)
	{
		auto hr = queue->Signal(fences[idx].Get(), fence_values[idx]);
		if (FAILED(hr))
		{
			throw std::runtime_error("Failed to signal command queue.");
		}
	}

	inline void Wait(const std::uint8_t idx)
	{
		if (fences[idx]->GetCompletedValue() < fence_values[idx])
		{
			// we have the fence create an event which is signaled once the fence's current value is "fenceValue"
			auto hr = fences[idx]->SetEventOnCompletion(fence_values[idx], fence_event);
			if (FAILED(hr))
			{
				throw std::runtime_error("Failed to set fence event.");
			}

			WaitForSingleObject(fence_event, INFINITE);
		}
	}

	ComPtr<ID3D12Fence>* fences;
	HANDLE fence_event;
	UINT64* fence_values;
};

/*! Macro to easily name D3D12 resources.
 * @param r The `D3D12Resource` to name.
 * @param n The name as wstring.
 */
#ifdef _DEBUG
#define NAME_D3D12RESOURCE(r, n) auto temp = std::string(__FILE__); r->SetName(std::wstring(std::wstring(n) + L" (line: " + std::to_wstring(__LINE__) + L" file: " + std::wstring(temp.begin(), temp.end())).c_str());
#else
#define NAME_D3D12RESOURCE(r, n) r->SetName(n);
#endif

// Template Function Implementations

template<unsigned int N>
[[nodiscard]] ComPtr<ID3D12CommandAllocator>* D3D12Viewer::CreateCommandAllocators()
{
	auto allocators = new ComPtr<ID3D12CommandAllocator>[N];

	// Create the allocators
	for (int i = 0; i < N; i++)
	{
		HRESULT hr = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocators[i]));
		if (FAILED(hr))
		{
			throw std::runtime_error("Failed to create command allocator");
		}

		NAME_D3D12RESOURCE(allocators[i], L"CommandList Allocator")
	}

	return allocators;
}

template<const std::uint16_t N>
[[nodiscard]] std::array<ComPtr<ID3D12Resource>, N> D3D12Viewer::GetRenderTargetsFromSwapChain(ComPtr<IDXGISwapChain4> swap_chain)
{
	std::array<ComPtr<ID3D12Resource>, N> render_targets;

	for (std::uint16_t i = 0; i < N; i++)
	{
		HRESULT hr = swap_chain->GetBuffer(i, IID_PPV_ARGS(&render_targets[i]));
		if (FAILED(hr))
		{
			throw "Failed to get swap chain buffer.";
		}
		render_targets[i]->SetName(L"Back buffer");
	}

	return render_targets;
}

template<const std::uint16_t N>
void D3D12Viewer::CreateRTVsFromResourceArray(std::array<ComPtr<ID3D12Resource>, N>& render_targets, CD3DX12_CPU_DESCRIPTOR_HANDLE desc_handle)
{
	for (std::uint16_t i = 0; i < N; i++)
	{
		m_device->CreateRenderTargetView(render_targets[i].Get(), nullptr, desc_handle);
		desc_handle.Offset(1, rtv_increment_size);
	}
}
