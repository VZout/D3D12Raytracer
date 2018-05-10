#pragma once

class ImGuiManager
{
	using imgui_func = std::function<void()>;
	// I recommend a 4th variable to store a catagory for the tool window. To organize the menu bar.
	using imgui_func_tuple = std::tuple<imgui_func, std::string, bool>;
public:
	ImGuiManager() = default;
	~ImGuiManager() = default;

	ImGuiManager(const rhs&) = delete;
	ImGuiManager& operator=(const rhs&) = delete;
	ImGuiManager(rhs&&) = delete;
	ImGuiManager& operator=(rhs&&) = delete;

	/* Initializes ImGui for your renderer */
	void Init(ComPtr<ID3D12Device>, ComPtr<ID3D12DescriptorHeap>);

	/* Renders ImGui and executes the registered functions.
	 * Also is able to render a menu bar listing all the tools and gives you the ability to close and open them from a nice view.
	 */
	void Render();

	/* Creates a tuple and puts it in the `m_registered_tools` func for future use. */
	void RegisterImGuiTool(imgui_func func, std::string const& panel_name, bool default_open = false);

private:
	std::vector<imgui_func_tuple> m_registered_tools;
};
