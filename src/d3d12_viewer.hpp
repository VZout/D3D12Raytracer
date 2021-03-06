#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <array>
#include <vector>
#include <d3dcompiler.h>
#include <variant>

#include "viewer.hpp"
#include "vec.hpp"
#include "d3dx12.hpp"
#include "../structs.hlsl"

using Microsoft::WRL::ComPtr;

struct RenderTexture
{
	ComPtr<ID3D12Resource> resource;
	ComPtr<ID3D12Resource> staging_resource;
	size_t bytes_per_row;
	std::uint8_t* address;
	D3D12_RESOURCE_DESC texture_desc;
};

class D3D12Viewer : public Viewer
{
	friend struct FenceObject;
	friend class D3D12RayTracer;
	friend class CPURayTracer;

	using VertexBuffer = std::tuple<ComPtr<ID3D12Resource>, ComPtr<ID3D12Resource>, D3D12_VERTEX_BUFFER_VIEW>;

public:
	explicit D3D12Viewer(Application& app);
	~D3D12Viewer() override;

	void NewFrame() override;
	void Present() override;

	static const D3D_FEATURE_LEVEL m_feature_level = D3D_FEATURE_LEVEL_12_1;
	static const DXGI_FORMAT m_back_buffer_format = DXGI_FORMAT_R8G8B8A8_UNORM;
	const std::string m_shader_model = "_5_0";
	static const bool m_shader_optimization = true;
	static const bool m_shader_debug = true;
	static const bool m_disable_gpu_timeout = true;
	static const std::uint8_t num_back_buffers = 2;
	static const bool m_use_vsync = false;
	const float m_clear_color[4] = { 0, 0, 0, 255 };

	/*! Used to fill a ImFGui window with adapter information */
	void ImGui_RenderSystemInfo();

private:
	ComPtr<IDXGIFactory5> m_factory;
	ComPtr<ID3D12Device> m_device;
	ComPtr<IDXGIAdapter4> m_adapter;
#ifdef _DEBUG
	DXGI_ADAPTER_DESC1 adapter_desc;
	SYSTEM_INFO system_info;
#endif
	ComPtr<IDXGISwapChain4> m_swap_chain;
	ComPtr<ID3D12CommandQueue> m_cmd_queue;
#ifdef ENABLE_IMGUI
	ComPtr<ID3D12DescriptorHeap> imgui_descriptor_heap;
#endif
	std::array<ComPtr<ID3D12Resource>, num_back_buffers> m_render_targets;
	ComPtr<ID3D12DescriptorHeap> m_render_target_view_heap;
	ComPtr<ID3D12PipelineState> m_pipeline;
	ComPtr<ID3D12RootSignature> m_root_signature;
	ComPtr<ID3D12DescriptorHeap> m_main_srv_desc_heap;
#ifdef _DEBUG
	ComPtr<ID3D12Debug1> debug_controller;
#endif

	RenderTexture* m_render_texture;

	std::uint32_t m_rtv_increment_size;
	std::uint32_t m_dsv_increment_size;
	std::uint32_t m_cbv_srv_uav_increment_size;
	std::uint32_t m_sampler_increment_size;

	std::uint8_t m_frame_idx;

	ComPtr<ID3D12CommandAllocator>* m_cmd_allocators;
	ComPtr<ID3D12GraphicsCommandList2> m_cmd_list;

	std::pair<D3D12_VIEWPORT, D3D12_RECT> m_viewport;

	FenceObject* fence;
	VertexBuffer screen_quad_vb;

	void SetupD3D12();
	void SetupImGui();
	void SetupSwapChain(std::uint16_t width, std::uint16_t height);

	void CreateFactory();
	void FindCompatibleAdapter();
	void CreateDevice();

	/*! Returns a basic root signature to render the ray tracer's output on a screen quad */
	[[nodiscard]] ComPtr<ID3D12RootSignature> CreateBasicRootSignature();
	/*! Returns a basic pipeline state for rendering a screen  quad
	 * @param root_signature The root signature return by `D3D12Viewer::CreateBasicRootSignature`.
	 */
	[[nodiscard]] ComPtr<ID3D12PipelineState> CreateBasicPipelineState(ComPtr<ID3D12RootSignature>& root_signature);

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
	[[nodiscard]] inline std::wstring GetUTF16(std::string_view const str, int codepage);
	[[nodiscard]] inline std::variant<std::pair<ID3DBlob*, D3D12_SHADER_BYTECODE>, std::string> LoadShader(std::string_view path, std::string_view entry, std::string_view type);
	[[nodiscard]] ComPtr<ID3D12DescriptorHeap> CreateSRVHeap(std::uint8_t num);
	[[nodiscard]] VertexBuffer CreateVertexBuffer(std::vector<fm::vec3> vertices);
	template<const std::uint16_t N>
	[[nodiscard]] std::pair<std::array<ComPtr<ID3D12Resource>, N>, std::array<UINT8*, N>> CreateConstantBuffer(size_t unaligned_size);
	template<const std::uint16_t N>
	[[nodiscard]] std::pair<std::array<ComPtr<ID3D12Resource>, N>, std::array<UINT8*, N>> CreateStructuredBuffer(size_t unaligned_size);
	template<const std::uint16_t N>
	[[nodiscard]] std::pair<std::array<ComPtr<ID3D12Resource>, N>, std::array<UINT8*, N>> CreateByteAddressBuffer(size_t unaligned_size);
	template<const std::uint16_t N>
	[[nodiscard]] void CreateStructuredBufferSRV(std::array<ComPtr<ID3D12Resource>, N> buffer, CD3DX12_CPU_DESCRIPTOR_HANDLE srv_handle, int count, int stride);
	template<const std::uint16_t N>
	[[nodiscard]] void CreateByteAddressBufferSRV(std::array<ComPtr<ID3D12Resource>, N> buffer, CD3DX12_CPU_DESCRIPTOR_HANDLE srv_handle, int count);
	[[nodiscard]] RenderTexture* CreateRenderTexture(unsigned int width, unsigned int height, DXGI_FORMAT format, CD3DX12_CPU_DESCRIPTOR_HANDLE srv_handle);
	void UpdateRenderTexture(ComPtr<ID3D12GraphicsCommandList> cmd_list, RenderTexture* texture, BYTE* data);
};

struct FenceObject
{
	/*! Creates fence resources, fence values and a fence event.
	 *
	 * @param viewer The owning Viewer.
	 * @param num Amount of fence values.
	 */
	FenceObject(D3D12Viewer& viewer, const std::uint8_t num);

	/*! Increments the fence value corresponding to the index. */
	inline void Increment(std::uint8_t idx);
	/*! Signal a command queue. */
	inline void Signal(ComPtr<ID3D12CommandQueue> queue, std::uint8_t idx);
	/*! Wait for the fence value corresponding to the index to unblock. */
	inline void Wait(const std::uint8_t idx);

	ComPtr<ID3D12Fence>* fences;
	HANDLE fence_event;
	UINT64* fence_values;
};

/*! Macro to easily name D3D12 resources.
 * @param r The `D3D12Resource` to name.
 * @param n The name as wstring.
 */
#ifdef _DEBUG
#define NAME_D3D12RESOURCE(r, n) auto temp = std::string(__FILE__); \
r->SetName(std::wstring(std::wstring(n) + L" (line: " + std::to_wstring(__LINE__) + L" file: " + std::wstring(temp.begin(), temp.end())).c_str());
#else
#define NAME_D3D12RESOURCE(r, n) r->SetName(n);
#endif

#define GET_VB_RESOURCE(vb) std::get<0>(vb)
#define GET_VB_UPLOAD_RESOURCE(vb) std::get<1>(vb)
#define GET_VB_VIEW(vb) std::get<2>(vb)
#define GET_CB_ADDRESS(cb, idx) cb.second[idx]

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
		desc_handle.Offset(1, m_rtv_increment_size);
	}
}

[[nodiscard]] inline std::variant<std::pair<ID3DBlob*, D3D12_SHADER_BYTECODE>, std::string> D3D12Viewer::LoadShader(std::string_view path, std::string_view entry, std::string_view type)
{
	D3D_SHADER_MACRO defines[] = { "GPU", nullptr, nullptr, nullptr };

	ID3DBlob* shader;
	ID3DBlob* error = nullptr;
	UINT flags0 = 0;
	flags0 |= m_shader_optimization ? D3DCOMPILE_OPTIMIZATION_LEVEL3 : D3DCOMPILE_SKIP_OPTIMIZATION;
	if (m_shader_debug) flags0 |=  D3DCOMPILE_DEBUG;
	HRESULT hr = D3DCompileFromFile(GetUTF16(path, CP_UTF8).c_str(),
		defines,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entry.data(),
		type.data(),
		flags0,
		0,
		&shader,
		&error);
	if (FAILED(hr))
	{
		if (!error)
			return std::string("Couldn't load file: " ).append(path);
		return std::string((char*)error->GetBufferPointer());
	}

	D3D12_SHADER_BYTECODE bytecode = {};
	bytecode.BytecodeLength = shader->GetBufferSize();
	bytecode.pShaderBytecode = shader->GetBufferPointer();

	return std::make_pair(shader, bytecode);
}

template<const std::uint16_t N>
[[nodiscard]] std::pair<std::array<ComPtr<ID3D12Resource>, N>, std::array<UINT8*, N>> D3D12Viewer::CreateConstantBuffer(size_t unaligned_size)
{
	unsigned int mul_size = (unaligned_size + 255) & ~255;

	std::array<ComPtr<ID3D12Resource>, N> const_buffers;
	std::array<UINT8*, N> const_buffer_adresses;

	for (unsigned int i = 0; i < N; ++i)
	{
		HRESULT hr = m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // this heap will be used to upload the constant buffer data
			D3D12_HEAP_FLAG_NONE, // no flags
			&CD3DX12_RESOURCE_DESC::Buffer(mul_size), // size of the resource heap. Must be a multiple of 64KB for single-textures and constant buffers
			D3D12_RESOURCE_STATE_GENERIC_READ, // will be data that is read from so we keep it in the generic read state
			nullptr, // we do not have use an optimized clear value for constant buffers
			IID_PPV_ARGS(&const_buffers[i]));
		const_buffers[i]->SetName(L"Constant Buffer Upload Resource Heap");
		if (FAILED(hr))
		{
			throw std::runtime_error("Failed to create constant buffer resource");
		}

		CD3DX12_RANGE readRange(0, 0);
		hr = const_buffers[i]->Map(0, &readRange, reinterpret_cast<void**>(&const_buffer_adresses[i]));
		if (FAILED(hr))
		{
			throw std::runtime_error("Failed to map constant buffer");
		}
	}

	return { const_buffers, const_buffer_adresses };
}

template<const std::uint16_t N>
[[nodiscard]] std::pair<std::array<ComPtr<ID3D12Resource>, N>, std::array<UINT8*, N>> D3D12Viewer::CreateStructuredBuffer(size_t unaligned_size)
{
	unsigned int mul_size = (unaligned_size + 255) & ~255;

	std::array<ComPtr<ID3D12Resource>, N> const_buffers;
	std::array<UINT8*, N> const_buffer_adresses;

	for (unsigned int i = 0; i < N; ++i)
	{
		HRESULT hr = m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // this heap will be used to upload the constant buffer data
			D3D12_HEAP_FLAG_NONE, // no flags
			&CD3DX12_RESOURCE_DESC::Buffer(mul_size), // size of the resource heap. Must be a multiple of 64KB for single-textures and constant buffers
			D3D12_RESOURCE_STATE_GENERIC_READ, // will be data that is read from so we keep it in the generic read state
			nullptr, // we do not have use an optimized clear value for constant buffers
			IID_PPV_ARGS(&const_buffers[i]));
		const_buffers[i]->SetName(L"Structured Buffer Upload Resource Heap");
		if (FAILED(hr))
		{
			throw std::runtime_error("Failed to create constant buffer resource");
		}

		CD3DX12_RANGE readRange(0, 0);
		hr = const_buffers[i]->Map(0, &readRange, reinterpret_cast<void**>(&const_buffer_adresses[i]));
		if (FAILED(hr))
		{
			throw std::runtime_error("Failed to map constant buffer");
		}
	}

	return { const_buffers, const_buffer_adresses };
}

template<const std::uint16_t N>
[[nodiscard]] std::pair<std::array<ComPtr<ID3D12Resource>, N>, std::array<UINT8*, N>> D3D12Viewer::CreateByteAddressBuffer(size_t unaligned_size)
{
	unsigned int mul_size = (unaligned_size + 255) & ~255;

	std::array<ComPtr<ID3D12Resource>, N> const_buffers;
	std::array<UINT8*, N> const_buffer_adresses;

	for (unsigned int i = 0; i < N; ++i)
	{
		HRESULT hr = m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // this heap will be used to upload the constant buffer data
			D3D12_HEAP_FLAG_NONE, // no flags
			&CD3DX12_RESOURCE_DESC::Buffer(mul_size), // size of the resource heap. Must be a multiple of 64KB for single-textures and constant buffers
			D3D12_RESOURCE_STATE_GENERIC_READ, // will be data that is read from so we keep it in the generic read state
			nullptr, // we do not have use an optimized clear value for constant buffers
			IID_PPV_ARGS(&const_buffers[i]));
		const_buffers[i]->SetName(L"byte Buffer Upload Resource Heap");
		if (FAILED(hr))
		{
			throw std::runtime_error("Failed to create byte buffer resource");
		}

		CD3DX12_RANGE readRange(0, 0);
		hr = const_buffers[i]->Map(0, &readRange, reinterpret_cast<void**>(&const_buffer_adresses[i]));
		if (FAILED(hr))
		{
			throw std::runtime_error("Failed to map byte buffer");
		}
	}

	return { const_buffers, const_buffer_adresses };
}

template<const std::uint16_t N>
[[nodiscard]] void D3D12Viewer::CreateStructuredBufferSRV(std::array<ComPtr<ID3D12Resource>, N> buffer, CD3DX12_CPU_DESCRIPTOR_HANDLE srv_handle, int count, int stride)
{
	for (auto i = 0; i < N; i++)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
		srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srv_desc.Buffer.FirstElement = 0;
		srv_desc.Buffer.NumElements = count;
		srv_desc.Buffer.StructureByteStride = stride;
		srv_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		m_device->CreateShaderResourceView(buffer[i].Get(), &srv_desc, srv_handle);
		srv_handle.Offset(1, m_cbv_srv_uav_increment_size);
	}
}

template<const std::uint16_t N>
[[nodiscard]] void D3D12Viewer::CreateByteAddressBufferSRV(std::array<ComPtr<ID3D12Resource>, N> buffer, CD3DX12_CPU_DESCRIPTOR_HANDLE srv_handle, int count)
{
	for (auto i = 0; i < N; i++)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
		srv_desc.Format = DXGI_FORMAT_R32_TYPELESS;
		srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srv_desc.Buffer.NumElements = count;
		srv_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;

		m_device->CreateShaderResourceView(buffer[i].Get(), &srv_desc, srv_handle);
		srv_handle.Offset(1, m_cbv_srv_uav_increment_size);
	}
}

[[nodiscard]] inline unsigned int SizeOfFormat(const DXGI_FORMAT format)
{
	switch (format)
	{
	case DXGI_FORMAT_R32G32B32A32_TYPELESS:
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
	case DXGI_FORMAT_R32G32B32A32_UINT:
	case DXGI_FORMAT_R32G32B32A32_SINT:
		return 16;

	case DXGI_FORMAT_R32G32B32_TYPELESS:
	case DXGI_FORMAT_R32G32B32_FLOAT:
	case DXGI_FORMAT_R32G32B32_UINT:
	case DXGI_FORMAT_R32G32B32_SINT:
		return 12;

	case DXGI_FORMAT_R16G16B16A16_TYPELESS:
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
	case DXGI_FORMAT_R16G16B16A16_UNORM:
	case DXGI_FORMAT_R16G16B16A16_UINT:
	case DXGI_FORMAT_R16G16B16A16_SNORM:
	case DXGI_FORMAT_R16G16B16A16_SINT:
	case DXGI_FORMAT_R32G32_TYPELESS:
	case DXGI_FORMAT_R32G32_FLOAT:
	case DXGI_FORMAT_R32G32_UINT:
	case DXGI_FORMAT_R32G32_SINT:
	case DXGI_FORMAT_R32G8X24_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
		return 8;

	case DXGI_FORMAT_R10G10B10A2_TYPELESS:
	case DXGI_FORMAT_R10G10B10A2_UNORM:
	case DXGI_FORMAT_R10G10B10A2_UINT:
	case DXGI_FORMAT_R11G11B10_FLOAT:
	case DXGI_FORMAT_R8G8B8A8_TYPELESS:
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_R8G8B8A8_UINT:
	case DXGI_FORMAT_R8G8B8A8_SNORM:
	case DXGI_FORMAT_R8G8B8A8_SINT:
	case DXGI_FORMAT_R16G16_TYPELESS:
	case DXGI_FORMAT_R16G16_FLOAT:
	case DXGI_FORMAT_R16G16_UNORM:
	case DXGI_FORMAT_R16G16_UINT:
	case DXGI_FORMAT_R16G16_SNORM:
	case DXGI_FORMAT_R16G16_SINT:
	case DXGI_FORMAT_R32_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_R32_FLOAT:
	case DXGI_FORMAT_R32_UINT:
	case DXGI_FORMAT_R32_SINT:
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8X8_UNORM:
		return 4;

	case DXGI_FORMAT_R8G8_TYPELESS:
	case DXGI_FORMAT_R8G8_UNORM:
	case DXGI_FORMAT_R8G8_UINT:
	case DXGI_FORMAT_R8G8_SNORM:
	case DXGI_FORMAT_R8G8_SINT:
	case DXGI_FORMAT_R16_TYPELESS:
	case DXGI_FORMAT_R16_FLOAT:
	case DXGI_FORMAT_D16_UNORM:
	case DXGI_FORMAT_R16_UNORM:
	case DXGI_FORMAT_R16_UINT:
	case DXGI_FORMAT_R16_SNORM:
	case DXGI_FORMAT_R16_SINT:
	case DXGI_FORMAT_B5G6R5_UNORM:
	case DXGI_FORMAT_B5G5R5A1_UNORM:
		return 2;

	case DXGI_FORMAT_R8_TYPELESS:
	case DXGI_FORMAT_R8_UNORM:
	case DXGI_FORMAT_R8_UINT:
	case DXGI_FORMAT_R8_SNORM:
	case DXGI_FORMAT_R8_SINT:
	case DXGI_FORMAT_A8_UNORM:
		return 1;

		// Compressed format; http://msdn2.microsoft.com/en-us/library/bb694531(VS.85).aspx
	case DXGI_FORMAT_BC2_TYPELESS:
	case DXGI_FORMAT_BC2_UNORM:
	case DXGI_FORMAT_BC2_UNORM_SRGB:
	case DXGI_FORMAT_BC3_TYPELESS:
	case DXGI_FORMAT_BC3_UNORM:
	case DXGI_FORMAT_BC3_UNORM_SRGB:
	case DXGI_FORMAT_BC5_TYPELESS:
	case DXGI_FORMAT_BC5_UNORM:
	case DXGI_FORMAT_BC5_SNORM:
		return 16;

		// Compressed format; http://msdn2.microsoft.com/en-us/library/bb694531(VS.85).aspx
	case DXGI_FORMAT_R1_UNORM:
	case DXGI_FORMAT_BC1_TYPELESS:
	case DXGI_FORMAT_BC1_UNORM:
	case DXGI_FORMAT_BC1_UNORM_SRGB:
	case DXGI_FORMAT_BC4_TYPELESS:
	case DXGI_FORMAT_BC4_UNORM:
	case DXGI_FORMAT_BC4_SNORM:
		return 8;

		// Compressed format; http://msdn2.microsoft.com/en-us/library/bb694531(VS.85).aspx
	case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
		return 4;

		// These are compressed, but bit-size information is unclear.
	case DXGI_FORMAT_R8G8_B8G8_UNORM:
	case DXGI_FORMAT_G8R8_G8B8_UNORM:
		return 4;

	case DXGI_FORMAT_UNKNOWN:
	default:
		return 0;
	}
}