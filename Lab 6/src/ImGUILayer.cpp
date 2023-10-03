#include <ImGUILayer.hpp>
#include <core/include/Random.hpp>

#include <numbers>
#include <iostream>
#include <algorithm>

#include <thread>
#include <imgui.h>

#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_dx11.h>

#include <omp.h>

using namespace retro;

namespace
{
    void DisplayBoolColored(const char* label, bool value)
    {
        ImVec4 color = value ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f);

        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::Text("%s: %s", label, value ? "true" : "false");
        ImGui::PopStyleColor();
    }

    bool DrawButtonConditionally(const std::string& label, bool disabled, const std::string& hint)
    {
        if (disabled)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
            ImGui::Button(label.c_str());
            ImGui::PopStyleVar();

            if (!hint.empty() && (ImGui::IsItemHovered() || ImGui::IsItemActive()))
            {
                ImGui::SetTooltip("%s", hint.c_str());
            }

            return false;
        }
        else
        {
            return ImGui::Button(label.c_str());
        }
    }

    template<typename T>
    T linear(T min, T max, T value)
    {
        return ((value - min) / (max - min));
    }

    template<typename T, typename E, typename F, typename... Args>
    void DrawColorLerp(T min, T max, T value, E&& easing, ImVec4 min_color, ImVec4 max_color, F&& func, Args&&... args)
    {
        ImVec4 color;

        if (value > max)
        {
            color = max_color;
        }
        else if (value < min)
        {
            color = min_color;
        }
        else
        {
            auto t = static_cast<float>(easing(min, max, value));

            color.x = min_color.x + t * (max_color.x - min_color.x);
            color.y = min_color.y + t * (max_color.y - min_color.y);
            color.z = min_color.z + t * (max_color.z - min_color.z);
            color.w = 1.0;
        }

        ImGui::PushStyleColor(ImGuiCol_Text, color);
        func(std::forward<Args>(args)...);
        ImGui::PopStyleColor();
    }
}

void ImGUILayer::OnAttach()
{
    omp_set_nested(1);

    m_window = std::make_unique<graphics::Window>("Lab 4 by Retro52", 1280, 720);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplSDL2_InitForD3D(m_window->GetWindowHandler());
    ImGui_ImplDX11_Init(m_window->GetDirectXDevice(), m_window->GetDirectXDeviceContext());

    // Setting up a color scheme (feel free to customize these colors)
    style.WindowPadding = ImVec2(15, 15);
    style.WindowRounding = 5.0f;
    style.FramePadding = ImVec2(5, 5);
    style.FrameRounding = 4.0f;
    style.ItemSpacing = ImVec2(12, 8);
    style.ItemInnerSpacing = ImVec2(8, 6);
    style.IndentSpacing = 25.0f;
    style.ScrollbarSize = 15.0f;
    style.ScrollbarRounding = 9.0f;
    style.GrabMinSize = 5.0f;
    style.GrabRounding = 3.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.09f, 0.09f, 0.09f, 0.94f);
    colors[ImGuiCol_Header] = ImVec4(0.28f, 0.28f, 0.28f, 0.75f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.28f, 0.28f, 0.28f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    colors[ImGuiCol_Text] = ImVec4(0.86f, 0.93f, 0.89f, 0.78f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.86f, 0.93f, 0.89f, 0.28f);
}

bool ImGUILayer::OnUpdate(ts delta)
{
    for (const auto& event : m_window->PollEvents())
    {
        core::EventsPoll::AddEvent(event);
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplSDL2_NewFrame();

    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport();

    try
    {
        Render();
    }
    catch(const std::exception& e)
    {
        std::cout << "Error! Exception details: " << e.what() << std::endl;
    }

    ImGui::Render();
    const float clear_color_with_alpha[4] = { 0.0F, 0.0F, 0.0F, 0.0F };

    auto target_view = m_window->GetDirectXRenderTargetView();
    m_window->GetDirectXDeviceContext()->OMSetRenderTargets(1, &target_view, nullptr);
    m_window->GetDirectXDeviceContext()->ClearRenderTargetView(m_window->GetDirectXRenderTargetView(), clear_color_with_alpha);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    const auto& io = ImGui::GetIO();

    // Update and Render additional Platform Windows
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }

    m_window->GetDirectXSwapChain()->Present(0, 0);

    return true;
}

void ImGUILayer::OnDetach()
{
    std::cout << "Layer removed: " << std::endl;
}

bool ImGUILayer::OnEvent(const core::Event &event)
{
    ImGui_ImplSDL2_ProcessEvent(&event);

    if (event.type == SDL_QUIT)
    {
        return false;
    }

    return true;
}

enum class CellState
{
    NONE,
    WOLF,
    RABBIT
};

void Simulate(auto& grid)
{
    std::vector<std::vector<CellState>> newGrid = grid;

    int dx[] = {-1, -1, -1, 0, 1, 1, 1, 0};
    int dy[] = {-1, 0, 1, 1, 1, 0, -1, -1};

    for (int i = 0; i < newGrid.size(); i++)
    {
        for (int j = 0; j < newGrid.at(i).size(); j++)
        {
            int rabbitsAround = 0;
            int wolvesAround = 0;

            for (int dir = 0; dir < 8; dir++)
            {
                int ni = i + dx[dir];
                int nj = j + dy[dir];

                if (ni >= 0 && ni < newGrid.at(i).size() && nj >= 0 && nj < newGrid.at(i).size())
                {
                    if (grid.at(ni).at(nj) == CellState::RABBIT)
                    {
                        rabbitsAround++;
                    }
                    else if (grid.at(ni).at(nj) == CellState::WOLF)
                    {
                        wolvesAround++;
                    }
                }
            }

            // Apply the rules
            if (grid.at(i).at(j) == CellState::WOLF)
            {
                if (rabbitsAround > 0)
                {
                    // Wolf eats a rabbit
                    newGrid.at(i).at(j) = CellState::WOLF;
                }
                else if (wolvesAround < 2 || wolvesAround > 3)
                {
                    // Wolf dies due to loneliness or overcrowding
                    newGrid.at(i).at(j) = CellState::NONE;
                }
            }
            else if (grid.at(i).at(j) == CellState::RABBIT)
            {
                if (rabbitsAround < 2 || rabbitsAround > 4)
                {
                    // Rabbit dies due to loneliness or overcrowding
                    newGrid.at(i).at(j) = CellState::NONE;
                }
            }
            else
            {
                // Empty cell
                if (rabbitsAround == 3)
                {
                    // Cell becomes a rabbit
                    newGrid.at(i).at(j) = CellState::RABBIT;
                }
                else if (wolvesAround == 3)
                {
                    // Cell becomes a wolf
                    newGrid.at(i).at(j) = CellState::WOLF;
                }
            }
        }
    }

    grid = newGrid;
}

void ImGUILayer::Render()
{
    static int gridSize = 20;

    static std::unordered_map<CellState, ImVec4> cell_colors =
        {
                { CellState::NONE, ImVec4(0.0f, 0.0f, 0.0f, 1.0f)},
                { CellState::WOLF, ImVec4(0.49f, 0.49f, 0.49f, 1.0f)},
                { CellState::RABBIT, ImVec4(1.0f, 0.0f, 1.0f, 1.0f)}
        };

    static int brushSize = 1;
    static CellState currentBrush = CellState::WOLF;
    static std::vector<std::vector<CellState>> grid;

    static thread::winthread test_thread;

    ImGui::Begin("Wolf v Rabbit");

    ImGui::SliderInt("Brush Size", &brushSize, 1, 5);

    CellState newBrush = currentBrush;
    if (currentBrush == CellState::NONE)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, cell_colors.at(CellState::NONE));
    }
    if (ImGui::Button("EMPTY"))
    {
        newBrush = CellState::NONE;
    }
    if (currentBrush == CellState::NONE)
    {
        ImGui::PopStyleColor();
    }
    ImGui::SameLine();

    if (currentBrush == CellState::WOLF)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, cell_colors.at(CellState::WOLF));
    }
    if (ImGui::Button("WOLF"))
    {
        newBrush = CellState::WOLF;
    }
    if (currentBrush == CellState::WOLF)
    {
        ImGui::PopStyleColor();
    }
    ImGui::SameLine();

    if (currentBrush == CellState::RABBIT)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, cell_colors.at(CellState::RABBIT));
    }
    if (ImGui::Button("RABBIT"))
    {
        newBrush = CellState::RABBIT;
    }
    if (currentBrush == CellState::RABBIT)
    {
        ImGui::PopStyleColor();
    }

    currentBrush = newBrush;

    // Color pickers for each state
    ImGui::ColorEdit3("NONE Color", (float*)&cell_colors.at(CellState::NONE));
    ImGui::ColorEdit3("WOLF Color", (float*)&cell_colors.at(CellState::WOLF));
    ImGui::ColorEdit3("RABBIT Color", (float*)&cell_colors.at(CellState::RABBIT));

    if (ImGui::Button("Clear"))
    {
        for (auto& row : grid)
        {
            for (auto& cell : row)
            {
                cell = CellState::NONE;
            }
        }
    }

    constexpr float cell_spacing = 1.0F;
    const auto sx = (ImGui::GetContentRegionAvail().x / static_cast<float>(gridSize)) - cell_spacing;
    const auto sy = (ImGui::GetContentRegionAvail().y / static_cast<float>(gridSize)) - cell_spacing;

    float cell_size = std::min(sx, sy);
    float total_cell_size = cell_size + cell_spacing;

    ImGui::Text("PropSize: %f %f %f", sx, sy, cell_size);
    ImGui::Text("RegAvail: %f %f", ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y);

    ImGui::DragInt("Grid size: ", &gridSize, 0.05F, 1, 100);

    ImVec2 mousePos = ImGui::GetMousePos();
    bool isMouseDown = ImGui::IsMouseDown(0);
    ImVec2 cursorStart = ImGui::GetCursorScreenPos();

    ImVec2 originalSpacing = ImGui::GetStyle().ItemSpacing;
    ImGui::GetStyle().ItemSpacing = ImVec2(2.0f, 2.0f);

    ImVec2 grid_window_size;
    grid_window_size.x = grid_window_size.y = total_cell_size * gridSize;

    static bool stop_simulation = false;

    if (test_thread.is_running() && ImGui::Button("Stop"))
    {
        stop_simulation = true;
    }
    if (ImGui::Button("Simulate"))
    {
        stop_simulation = false;
        test_thread.run(
            [=]()
            {
                while(!stop_simulation)
                {
                    Simulate(grid);

                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                }
            });
    }

    ImGui::BeginChild("GridChild", grid_window_size, false, ImGuiWindowFlags_HorizontalScrollbar);

    if (grid.size() < gridSize)
    {
        grid.resize(gridSize);
    }

    for (int i = 0; i < gridSize; i++)
    {
        if (grid.at(i).size() < gridSize)
        {
            grid.at(i).resize(gridSize);
        }
        for (int j = 0; j < gridSize; j++)
        {
            ImVec2 cellMin = ImVec2(cursorStart.x + j * total_cell_size, cursorStart.y + i * total_cell_size);
            ImVec2 cellMax = ImVec2(cellMin.x + cell_size, cellMin.y + cell_size);

            bool isInsideBrush =
                    mousePos.x >= (cellMin.x - cell_size * (brushSize - 1)) &&
                    mousePos.x <= (cellMax.x + cell_size * (brushSize - 1)) &&
                    mousePos.y >= (cellMin.y - cell_size * (brushSize - 1)) &&
                    mousePos.y <= (cellMax.y + cell_size * (brushSize - 1));

            if (isMouseDown && ImGui::IsItemActive() && isInsideBrush)
            {
                grid.at(i).at(j) = currentBrush;
            }
            CellState cellState = grid.at(i).at(j);

            ImVec4 color;
            if (cell_colors.find(cellState) == cell_colors.end())
            {
                std::cerr << "Unexpected cell state value: " << static_cast<int>(cellState) << std::endl;
                color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            }
            else
            {
                color = cell_colors.at(cellState);
            }

            ImGui::GetWindowDrawList()->AddRectFilled(cellMin, cellMax, ImGui::ColorConvertFloat4ToU32(color));
        }
    }

    ImGui::EndChild();

    ImGui::GetStyle().ItemSpacing = originalSpacing;

    ImGui::End();
}
