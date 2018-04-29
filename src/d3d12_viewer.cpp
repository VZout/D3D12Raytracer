#include "d3d12_viewer.hpp"

#include <cassert>
#include <iostream>

#include "window.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx12.h"

D3D12Viewer::D3D12Viewer(Application& app) : Viewer(app)
{
	SetupD3D12();
	SetupImGui();
	SetupSwapChain(600, 600);

	m_cmd_allocators = CreateCommandAllocators<num_back_buffers>();
	m_cmd_list = CreateGraphicsCommandList(m_cmd_allocators[m_frame_idx]);

	// Create fence and Signal so we can make sure our data uploaded during initialization is done.
	fence = new FenceObject(*this, num_back_buffers);
	fence->Signal(m_cmd_queue, m_frame_idx);

	// Get Back buffers and create SRV's to them.
	m_render_targets = GetRenderTargetsFromSwapChain<num_back_buffers>(m_swap_chain);
	m_render_target_view_heap = CreateRenderTargetViewHeap(num_back_buffers);
	CreateRTVsFromResourceArray(m_render_targets, (CD3DX12_CPU_DESCRIPTOR_HANDLE)m_render_target_view_heap->GetCPUDescriptorHandleForHeapStart());

	// Create Viewport and sciccor rect.
	m_viewport = CreateViewportAndRect(600, 600);
}

D3D12Viewer::~D3D12Viewer()
{
	fence->Wait(0);
	fence->Wait(1);
}

void D3D12Viewer::NewFrame()
{
	// Wait for command list to be available.
	fence->Wait(m_frame_idx);
	fence->Increment(m_frame_idx);

	// Reset command allocator
	auto hr = m_cmd_allocators[m_frame_idx]->Reset();
	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to reset cmd allocators");
	}
	// Reset command list
	hr = m_cmd_list->Reset(m_cmd_allocators[m_frame_idx].Get(), nullptr);
	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to reset cmd list");
	}

	CD3DX12_RESOURCE_BARRIER begin_transition = CD3DX12_RESOURCE_BARRIER::Transition(
		m_render_targets[m_frame_idx].Get(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	m_cmd_list->ResourceBarrier(1, &begin_transition);

	ImGui_ImplDX12_NewFrame(m_cmd_list.Get());

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(m_render_target_view_heap->GetCPUDescriptorHandleForHeapStart(), m_frame_idx, rtv_increment_size);
	m_cmd_list->ClearRenderTargetView(rtv_handle, m_clear_color, 0, nullptr);
	m_cmd_list->OMSetRenderTargets(1, &rtv_handle, false, nullptr);
	
	//m_cmd_list->RSSetViewports(1, &m_viewport.first); // set the viewports
	//m_cmd_list->RSSetScissorRects(1, &m_viewport.second); // set the scissor rects

	/*
	cmd_list->SetPipelineState(pipeline);
	cmd_list->SetGraphicsRootSignature(root_signature);

	cmd_list->SetDescriptorHeaps(1, &main_srv_desc_heap);

	cmd_list->SetGraphicsRootConstantBufferView(0, const_buffers[frame_idx]->GetGPUVirtualAddress());
	cmd_list->SetGraphicsRootDescriptorTable(1, main_srv_desc_heap->GetGPUDescriptorHandleForHeapStart());

	cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmd_list->IASetVertexBuffers(0, 1, &vertex_buffer_view);
	cmd_list->IASetIndexBuffer(&index_buffer_view);
	cmd_list->DrawIndexedInstanced(6, 1, 0, 0, 0);
	*/
}

void D3D12Viewer::Present()
{
	// Render ImGui.
	auto a = imgui_descriptor_heap.Get();
	m_cmd_list->SetDescriptorHeaps(1, &a);
	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData());

	// Close command lists
	CD3DX12_RESOURCE_BARRIER end_transition = CD3DX12_RESOURCE_BARRIER::Transition(
		m_render_targets[m_frame_idx].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT
	);
	m_cmd_list->ResourceBarrier(1, &end_transition);
	m_cmd_list->Close();

	// Execute command lists
	ID3D12CommandList* cmd_lists[1] = { m_cmd_list.Get() };
	m_cmd_queue->ExecuteCommandLists(1, cmd_lists);

	// GPU	Signal
	fence->Signal(m_cmd_queue, m_frame_idx);
	
	// Present the frame.
	m_swap_chain->Present(use_vsync, 0);
	m_frame_idx = static_cast<std::uint8_t>(m_swap_chain->GetCurrentBackBufferIndex());
}

void D3D12Viewer::SetupD3D12()
{
#ifdef _DEBUG
	// Setup debug layer
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller))))
	{
		debug_controller->SetEnableGPUBasedValidation(true);
		debug_controller->EnableDebugLayer();
	}
#endif

	CreateFactory();
	FindCompatibleAdapter();
	CreateDevice();

	// Init increment sizes
	rtv_increment_size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	dsv_increment_size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	cbv_srv_uav_increment_size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	sampler_increment_size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	// Create Global realtime queue.
	D3D12_COMMAND_QUEUE_DESC cmd_queue_desc = {
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		0,
		disable_gpu_timeout ? D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT : D3D12_COMMAND_QUEUE_FLAG_NONE
	};
	HRESULT hr = m_device->CreateCommandQueue(&cmd_queue_desc, IID_PPV_ARGS(&m_cmd_queue));
	if (FAILED(hr))
	{
		std::cout << "Failed to create command queue with global realtime priority. Trying \"HIGH\" instead" << std::endl;

		// Create high priority queue.
		cmd_queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH;
		hr = m_device->CreateCommandQueue(&cmd_queue_desc, IID_PPV_ARGS(&m_cmd_queue));
		if (FAILED(hr))
		{
			throw std::runtime_error("Failed to create command queue.");
		}
	}
}

void D3D12Viewer::SetupImGui()
{
	D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
	heap_desc.NumDescriptors = 10;
	heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	HRESULT hr = m_device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&imgui_descriptor_heap));
	NAME_D3D12RESOURCE(imgui_descriptor_heap, L"ImGui Descriptor Heap");

	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to create descriptor heap for render target views");
	}

	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
	ImGui_ImplDX12_Init(m_app.GetWindowHandle(), num_back_buffers, m_device.Get(),
		DXGI_FORMAT_R8G8B8A8_UNORM,
		imgui_descriptor_heap->GetCPUDescriptorHandleForHeapStart(),
		imgui_descriptor_heap->GetGPUDescriptorHandleForHeapStart());

	ImGui::StyleColorsLight();

	ImGui_ImplDX12_InvalidateDeviceObjects();
	ImGui_ImplDX12_CreateDeviceObjects();
}

void D3D12Viewer::SetupSwapChain(std::uint16_t width, std::uint16_t height)
{
	IDXGISwapChain1* temp_swap_chain;

	// Describe multisampling capabilities.
	DXGI_SAMPLE_DESC sample_desc = {};
	sample_desc.Count = 1;
	sample_desc.Quality = 0;

	// Describe the swap chain
	DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
	swap_chain_desc.Width = width;
	swap_chain_desc.Height = height;
	swap_chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swap_chain_desc.SampleDesc = sample_desc;
	swap_chain_desc.BufferCount = num_back_buffers;
	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swap_chain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	HRESULT hr = m_factory->CreateSwapChainForHwnd(
		m_cmd_queue.Get(),
		m_app.GetWindowHandle(),
		&swap_chain_desc,
		nullptr,
		nullptr,
		&temp_swap_chain
	);
	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to create swap chain.");
	}

	// Upcast IDXGISwapChain1 TO IDXGISwapChain4. i Believe this is allowed.
	m_swap_chain = static_cast<IDXGISwapChain4*>(temp_swap_chain);
	m_frame_idx = static_cast<std::uint8_t>(m_swap_chain->GetCurrentBackBufferIndex());
}

void D3D12Viewer::CreateFactory()
{
	HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&m_factory));
	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to create DXGIFactory.");
	}

	m_factory->MakeWindowAssociation(m_app.GetWindowHandle(), m_allow_fullscreen ? 0 : DXGI_MWA_NO_ALT_ENTER);
}

void D3D12Viewer::FindCompatibleAdapter()
{
	assert(m_factory);

	IDXGIAdapter1* adapter = nullptr;
	std::uint8_t adapter_idx = 0;

	// Find a compatible adapter.
	while (m_factory->EnumAdapters1(adapter_idx, &adapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		// Skip software adapters.
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			adapter_idx++;
			continue;
		}

		// Create a device to test if the adapter supports the specified feature level.
		HRESULT hr = D3D12CreateDevice(adapter, m_feature_level, _uuidof(ID3D12Device), nullptr);
		if (SUCCEEDED(hr))
		{
			break;
		}

		adapter_idx++;
	}

	if (adapter == nullptr)
	{
		throw std::runtime_error("No comaptible adapter found.");
	}

	// Upcast IDXGIAdapter1 TO IDXGIAdapter4. i Believe this is allowed.
	m_adapter = static_cast<IDXGIAdapter4*>(adapter);
}


void D3D12Viewer::CreateDevice()
{
	assert(m_factory);
	assert(m_adapter);

	// Actually create the device.
	HRESULT hr = D3D12CreateDevice(m_adapter.Get(), m_feature_level, IID_PPV_ARGS(&m_device));
	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to create device.");
	}
}

ComPtr<ID3D12GraphicsCommandList2> D3D12Viewer::CreateGraphicsCommandList(ComPtr<ID3D12CommandAllocator>& allocator)
{
	ComPtr<ID3D12GraphicsCommandList2> cmd_list;

	// Create the command lists
	HRESULT hr = m_device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		allocator.Get(),
		nullptr,
		IID_PPV_ARGS(&cmd_list)
	);
	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to create command list");
	}
	NAME_D3D12RESOURCE(cmd_list, L"Command list");
	cmd_list->Close();

	return cmd_list;
}

ComPtr<ID3D12DescriptorHeap> D3D12Viewer::CreateDepthStencilHeap(std::uint16_t num_buffers)
{
	ComPtr<ID3D12DescriptorHeap> heap;

	D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
	heap_desc.NumDescriptors = num_buffers;
	heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HRESULT hr = m_device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&heap));
	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to create descriptor heap for depth stencil buffers");
	}

	return heap;
}

ComPtr<ID3D12Resource> D3D12Viewer::CreateDepthStencilBuffer(D3D12_CPU_DESCRIPTOR_HANDLE desc_handle, fm::vec2 size)
{
	return CreateDepthStencilBuffer(desc_handle, size.x, size.y);
}

ComPtr<ID3D12Resource> D3D12Viewer::CreateDepthStencilBuffer(D3D12_CPU_DESCRIPTOR_HANDLE desc_handle, std::uint16_t width, std::uint16_t height)
{
	ComPtr<ID3D12Resource> buffer;

	D3D12_CLEAR_VALUE optimized_clear_value = {};
	optimized_clear_value.Format = DXGI_FORMAT_D32_FLOAT;
	optimized_clear_value.DepthStencil.Depth = 1.0f;
	optimized_clear_value.DepthStencil.Stencil = 0;

	HRESULT hr = m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&optimized_clear_value,
		IID_PPV_ARGS(&buffer)
	);
	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to create commited resource.");
	}
	buffer->SetName(L"Depth/Stencil Buffer");

	D3D12_DEPTH_STENCIL_VIEW_DESC view_desc = {};
	view_desc.Format = DXGI_FORMAT_D32_FLOAT;
	view_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	view_desc.Flags = D3D12_DSV_FLAG_NONE;

	m_device->CreateDepthStencilView(buffer.Get(), &view_desc, desc_handle);

	return buffer;
}

ComPtr<ID3D12DescriptorHeap> D3D12Viewer::CreateRenderTargetViewHeap(std::uint16_t num_buffers)
{
	ID3D12DescriptorHeap* heap;

	D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
	heap_desc.NumDescriptors = num_buffers;
	heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HRESULT hr = m_device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&heap));
	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to create descriptor heap for render target views");
	}

	return heap;
}

std::pair<D3D12_VIEWPORT, D3D12_RECT> D3D12Viewer::CreateViewportAndRect(std::uint16_t width, std::uint16_t height)
{
	// Define viewport.
	D3D12_VIEWPORT viewport;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = width;
	viewport.Height = height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	// Define scissor rect
	D3D12_RECT scissor_rect;
	scissor_rect.left = 0;
	scissor_rect.top = 0;
	scissor_rect.right = width;
	scissor_rect.bottom = height;

	return {viewport, scissor_rect };
}
