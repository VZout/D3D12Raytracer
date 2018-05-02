#include "d3d12_viewer.hpp"

#include <cassert>
#include <iostream>

#include "texture.hpp"
#include "window.hpp"
#ifdef ENABLE_IMGUI
#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx12.h"
#endif

D3D12Viewer::D3D12Viewer(Application& app) : Viewer(app)
{
	SetupD3D12();
	SetupImGui();
	SetupSwapChain(app.GetWidth(), app.GetHeight());

	m_cmd_allocators = CreateCommandAllocators<num_back_buffers>();
	m_cmd_list = CreateGraphicsCommandList(m_cmd_allocators[m_frame_idx]);

	// Create fence
	fence = new FenceObject(*this, num_back_buffers);

	// Get Back buffers and create SRV's to them.
	m_render_targets = GetRenderTargetsFromSwapChain<num_back_buffers>(m_swap_chain);
	m_render_target_view_heap = CreateRenderTargetViewHeap(num_back_buffers);
	CreateRTVsFromResourceArray(m_render_targets,(CD3DX12_CPU_DESCRIPTOR_HANDLE)m_render_target_view_heap->GetCPUDescriptorHandleForHeapStart());

	// Create Viewport and sciccor rect.
	m_viewport = CreateViewportAndRect(app.GetWidth(), app.GetHeight());

	// Create the basic pipeline and root signature.
	m_root_signature = CreateBasicRootSignature();
	m_pipeline = CreateBasicPipelineState(m_root_signature);

	// Create Shader resource view heap to store the raytraced texture.
	m_main_srv_desc_heap = CreateSRVHeap(1);

	// Create Screen Squad Vertex Buffer;
	std::vector<fm::vec3> vertices =
	{
		{ -1.f, -1.f, 0.f },
		{ 1.f, -1.f, 0.f },
		{ -1.f, 1.f, 0.f },
		{ 1.f, 1.f, 0.f }
	};

	screen_quad_vb = CreateVertexBuffer(vertices);

	// Execute cmd list so VB is uploaded
	m_cmd_list->Close();
	ID3D12CommandList* cmd_lists[1] = { m_cmd_list.Get() };
	m_cmd_queue->ExecuteCommandLists(1, cmd_lists);

	// Synchronize with GPU to make sure VB is uploaded
	fence->Signal(m_cmd_queue, m_frame_idx);

	m_render_texture = CreateRenderTexture(app.GetWidth(), app.GetHeight(), DXGI_FORMAT_R32G32B32A32_FLOAT, (CD3DX12_CPU_DESCRIPTOR_HANDLE)m_main_srv_desc_heap->GetCPUDescriptorHandleForHeapStart());
}

D3D12Viewer::~D3D12Viewer()
{
	fence->Wait(0);
	fence->Wait(1);
}

bool first = true;
#include "d3d12_ray_tracer.hpp"
void D3D12Viewer::NewFrame()
{
	// Wait for command list to be available.
	fence->Wait(m_frame_idx);
	fence->Increment(m_frame_idx);

	if (first)
	{
		//GET_VB_UPLOAD_RESOURCE(screen_quad_vb)();
		first = false;
	}

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

#ifdef ENABLE_IMGUI
	ImGui_ImplDX12_NewFrame(m_cmd_list.Get());
#endif

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(m_render_target_view_heap->GetCPUDescriptorHandleForHeapStart(),
	                                         m_frame_idx,
											 m_rtv_increment_size);
	m_cmd_list->ClearRenderTargetView(rtv_handle, m_clear_color, 0, nullptr);
	m_cmd_list->OMSetRenderTargets(1, &rtv_handle, false, nullptr);
	
	m_cmd_list->RSSetViewports(1, &m_viewport.first); // set the viewports
	m_cmd_list->RSSetScissorRects(1, &m_viewport.second); // set the scissor rects

	
	m_cmd_list->SetPipelineState(m_pipeline.Get());
	m_cmd_list->SetGraphicsRootSignature(m_root_signature.Get());

	auto teamp_heap = m_main_srv_desc_heap.Get();
	m_cmd_list->SetDescriptorHeaps(1, &teamp_heap);

	m_cmd_list->SetGraphicsRootDescriptorTable(1, m_main_srv_desc_heap->GetGPUDescriptorHandleForHeapStart());

	m_cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	m_cmd_list->IASetVertexBuffers(0, 1, &GET_VB_VIEW(screen_quad_vb));
}

void D3D12Viewer::Present()
{
	// Render ImGui.
#ifdef ENABLE_IMGUI
	auto teamp_heap = imgui_descriptor_heap.Get();
	m_cmd_list->SetDescriptorHeaps(1, &teamp_heap);
	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData());
#endif

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
	m_swap_chain->Present(m_use_vsync, 0);
	m_frame_idx = static_cast<std::uint8_t>(m_swap_chain->GetCurrentBackBufferIndex());
}

void D3D12Viewer::SetupD3D12()
{
#ifdef _DEBUG
	// Setup debug layer
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller))))
	{
		//debug_controller->SetEnableGPUBasedValidation(true);
		debug_controller->EnableDebugLayer();
	}
#endif

	CreateFactory();
	FindCompatibleAdapter();
	CreateDevice();

	// Init increment sizes
	m_rtv_increment_size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_dsv_increment_size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	m_cbv_srv_uav_increment_size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_sampler_increment_size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	// Create Global realtime queue.
	D3D12_COMMAND_QUEUE_DESC cmd_queue_desc = {
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		0,
		m_disable_gpu_timeout ? D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT : D3D12_COMMAND_QUEUE_FLAG_NONE
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
#ifdef ENABLE_IMGUI
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
#endif
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

ComPtr<ID3D12PipelineState> D3D12Viewer::CreateBasicPipelineState(ComPtr<ID3D12RootSignature>& root_signature)
{
	ComPtr<ID3D12PipelineState> pipeline;

	D3D12_BLEND_DESC blend_desc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	D3D12_DEPTH_STENCIL_DESC depth_stencil_state = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

	D3D12_RASTERIZER_DESC rasterize_desc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

	rasterize_desc.FrontCounterClockwise = true;
	DXGI_SAMPLE_DESC sampleDesc = { 1, 0 };

	std::vector<D3D12_INPUT_ELEMENT_DESC> input_layout = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_INPUT_LAYOUT_DESC input_layout_desc = {};
	input_layout_desc.NumElements = input_layout.size();
	input_layout_desc.pInputElementDescs = input_layout.data();

	auto vertex_shader = LoadShader("raytracer.vs", "main", "vs" + m_shader_model);
	auto pixel_shader = LoadShader("raytracer.ps", "main", "ps" + m_shader_model);

	if (std::holds_alternative<std::string>(vertex_shader))
	{
		auto msg = std::get<std::string>(vertex_shader);
		MessageBox(nullptr, TEXT((char*)msg.c_str()), NULL, MB_OK | MB_ICONERROR);
		std::exit(EXIT_FAILURE);
	}

	if (std::holds_alternative<std::string>(pixel_shader))
	{
		auto msg = std::get<std::string>(pixel_shader);
		MessageBox(nullptr, TEXT((char*)msg.c_str()), NULL, MB_OK | MB_ICONERROR);
		std::exit(EXIT_FAILURE);
	}

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};
	pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	pso_desc.SampleDesc = sampleDesc;
	pso_desc.SampleMask = 0xffffffff;
	pso_desc.RasterizerState = rasterize_desc;
	pso_desc.BlendState = blend_desc;
	pso_desc.NumRenderTargets = 1;
	pso_desc.pRootSignature = root_signature.Get();
	pso_desc.VS = std::get<std::pair<ID3DBlob*, D3D12_SHADER_BYTECODE>>(vertex_shader).second;
	pso_desc.PS = std::get<std::pair<ID3DBlob*, D3D12_SHADER_BYTECODE>>(pixel_shader).second;
	pso_desc.InputLayout = input_layout_desc;

	HRESULT hr = m_device->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&pipeline));
	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to create graphics pipeline");
	}
	pipeline->SetName(L"Raytracing pipeline object");

	return pipeline;
}

ComPtr<ID3D12RootSignature> D3D12Viewer::CreateBasicRootSignature()
{
	ComPtr<ID3D12RootSignature> root_signature;

	std::array<D3D12_STATIC_SAMPLER_DESC, 1> samplers;
	samplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	samplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	samplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	samplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	samplers[0].MipLODBias = 0;
	samplers[0].MaxAnisotropy = 0;
	samplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	samplers[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	samplers[0].MinLOD = 0.0f;
	samplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	samplers[0].ShaderRegister = 0;
	samplers[0].RegisterSpace = 0;
	samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	CD3DX12_DESCRIPTOR_RANGE desc_range;
	desc_range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	std::array<CD3DX12_ROOT_PARAMETER, 2> parameters;
	parameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	parameters[1].InitAsDescriptorTable(1, &desc_range, D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_ROOT_SIGNATURE_DESC root_signature_desc;
	root_signature_desc.Init(parameters.size(),
		parameters.data(),
		samplers.size(),
		samplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ID3DBlob* signature;
	ID3DBlob* error = nullptr;
	auto hr = D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &signature, &error); //TODO: FIX error parameter
	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to create a serialized root signature");
	}

	hr = m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&root_signature));
	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to create root signature");
	}
	NAME_D3D12RESOURCE(root_signature, L"Basic root signature");

	return root_signature;
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

	return { viewport, scissor_rect };
}

inline std::wstring D3D12Viewer::GetUTF16(std::string_view const str, int codepage)
{
	if (str.empty()) return std::wstring();
	int sz = MultiByteToWideChar((UINT)codepage, 0, &str[0], (int)str.size(), 0, 0);
	std::wstring retval(sz, 0);
	MultiByteToWideChar((UINT)codepage, 0, &str[0], (int)str.size(), &retval[0], sz);
	return retval;
}

ComPtr<ID3D12DescriptorHeap> D3D12Viewer::CreateSRVHeap(std::uint8_t num)
{
	ComPtr<ID3D12DescriptorHeap> heap;

	D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
	heap_desc.NumDescriptors = num;
	heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	HRESULT hr = m_device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&heap));
	NAME_D3D12RESOURCE(heap, L"SRV Heap")

	return heap;
}

D3D12Viewer::VertexBuffer D3D12Viewer::CreateVertexBuffer(std::vector<fm::vec3> vertices)
{
	auto vertex_buffer_size = vertices.size() * sizeof(decltype(vertices[0]));
	ComPtr<ID3D12Resource> buffer;
	ComPtr<ID3D12Resource> staging_buffer;
	D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view;

	m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vertex_buffer_size),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&buffer));
	buffer->SetName(L"Vertex Buffer Resource Heap");

	m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vertex_buffer_size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&staging_buffer));
	staging_buffer->SetName(L"Vertex Buffer Upload Resource Heap");

	// store vertex buffer in upload heap
	D3D12_SUBRESOURCE_DATA vertex_data = {};
	vertex_data.pData = reinterpret_cast<BYTE*>(vertices.data());
	vertex_data.RowPitch = vertex_buffer_size;
	vertex_data.SlicePitch = vertex_buffer_size;

	UpdateSubresources(m_cmd_list.Get(), buffer.Get(), staging_buffer.Get(), 0, 0, 1, &vertex_data);

	// transition the vertex buffer data from copy destination state to vertex buffer state
	m_cmd_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	// create a vertex buffer view for the rectangle. We get the GPU memory address to the vertex buffer using the GetGPUVirtualAddress() method
	vertex_buffer_view.BufferLocation = buffer->GetGPUVirtualAddress();
	vertex_buffer_view.StrideInBytes = sizeof(decltype(vertices[0]));
	vertex_buffer_view.SizeInBytes = vertex_buffer_size;

	return { buffer, staging_buffer, vertex_buffer_view };
}

RenderTexture* D3D12Viewer::CreateRenderTexture(unsigned int width, unsigned int height, DXGI_FORMAT format, CD3DX12_CPU_DESCRIPTOR_HANDLE srv_handle)
{
	RenderTexture* texture = new RenderTexture();

	D3D12_RESOURCE_DESC texture_desc = {};
	texture_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texture_desc.Alignment = 0;
	texture_desc.Width = width;
	texture_desc.Height = height;
	texture_desc.DepthOrArraySize = 1;
	texture_desc.MipLevels = 1;
	texture_desc.Format = format;
	texture_desc.SampleDesc.Count = 1;
	texture_desc.SampleDesc.Quality = 0;
	texture_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texture_desc.Flags = D3D12_RESOURCE_FLAG_NONE;
	texture->texture_desc = texture_desc;

	texture->bytes_per_row = width * SizeOfFormat(format);

	size_t texture_upload_buffer_size;
	m_device->GetCopyableFootprints(&texture_desc, 0, 1, 0, nullptr, nullptr, nullptr, &texture_upload_buffer_size);

	CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(texture_upload_buffer_size);

	HRESULT hr = m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texture_desc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&texture->resource));

	if (FAILED(hr))
		throw std::runtime_error("Couldn't create default texture");

	hr = m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&texture->staging_resource));

	if (FAILED(hr))
		throw std::runtime_error("Couldn't create upload texture");

	texture->resource->SetName(L"Render Texture");
	texture->staging_resource->SetName(L"Upload Render Texture");

	// Create SRV View
	unsigned int increment_size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srv_desc.Format = texture->texture_desc.Format;
	srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Texture2D.MipLevels = texture->texture_desc.MipLevels;
	srv_desc.Texture2D.MostDetailedMip = 0;

	m_device->CreateShaderResourceView(texture->resource.Get(), &srv_desc, srv_handle);

	return texture;
}

void D3D12Viewer::UpdateRenderTexture(ComPtr<ID3D12GraphicsCommandList> cmd_list, RenderTexture* texture, BYTE* data)
{
	cmd_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texture->resource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST));

	size_t texture_upload_buffer_size;
	size_t bytes_per_row;
	UINT num_rows;
	m_device->GetCopyableFootprints(&texture->texture_desc, 0, 1, 0, nullptr, &num_rows, &bytes_per_row, &texture_upload_buffer_size);

	D3D12_SUBRESOURCE_DATA image_data = {};
	image_data.pData = data;
	image_data.RowPitch = texture->bytes_per_row;
	image_data.SlicePitch = (bytes_per_row)* num_rows;

	UpdateSubresources(cmd_list.Get(), texture->resource.Get(), texture->staging_resource.Get(), 0, 0, 1, &image_data);
	cmd_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texture->resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
}

FenceObject::FenceObject(D3D12Viewer& viewer, const std::uint8_t num)
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

inline void FenceObject::Increment(std::uint8_t idx)
{
	fence_values[idx]++;
}

inline void FenceObject::Signal(ComPtr<ID3D12CommandQueue> queue, std::uint8_t idx)
{
	auto hr = queue->Signal(fences[idx].Get(), fence_values[idx]);
	if (FAILED(hr))
	{
		throw std::runtime_error("Failed to signal command queue.");
	}
}

inline void FenceObject::Wait(const std::uint8_t idx)
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