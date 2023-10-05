#include <ImGUILayer.hpp>
#include <core/include/Random.hpp>

#include <mutex>
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

    enum class CellState
    {
        NONE,
        WOLF,
        RABBIT
    };

    std::string ToString(CellState state)
    {
        switch (state)
        {
            case CellState::NONE:
                return "EMPTY";
            case CellState::WOLF:
                return "WOLF";
            case CellState::RABBIT:
                return "RABBIT";
            default:
                return "UNKNOWN";
        }
    }

    void Simulate(std::vector<std::vector<CellState>>& grid)
    {
        std::vector<std::vector<CellState>> newGrid = grid;

        int dx[] = {-1, -1, -1, 0, 1, 1,  1,  0};
        int dy[] = {-1,  0,  1, 1, 1, 0, -1, -1};

#pragma omp parallel for shared(newGrid, dx, dy)
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

                    if (ni >= 0 && ni < newGrid.size() && nj >= 0 && nj < newGrid.at(ni).size())
                    {
                        if (newGrid.at(ni).at(nj) == CellState::RABBIT)
                        {
                            rabbitsAround++;
                        }
                        else if (newGrid.at(ni).at(nj) == CellState::WOLF)
                        {
                            wolvesAround++;
                        }
                    }
                }

                // Apply the rules
                if (newGrid.at(i).at(j) == CellState::WOLF)
                {
                    if (rabbitsAround > 0)
                    {
                        for (int dir = 0; dir < 8; dir++)
                        {
                            int ni = i + dx[dir];
                            int nj = j + dy[dir];

                            if (ni >= 0 && ni < newGrid.size() && nj >= 0 && nj < newGrid.at(ni).size())
                            {
                                if (newGrid.at(ni).at(nj) == CellState::RABBIT)
                                {
                                    // Wolf eats a rabbit
                                    newGrid.at(ni).at(nj) = CellState::NONE;
                                    break;
                                }
                            }
                        }
                    }
                    else if (wolvesAround < 2 || wolvesAround > 3)
                    {
                        // Wolf dies due to loneliness or overcrowding
                        newGrid.at(i).at(j) = CellState::NONE;
                    }
                }
                else if (newGrid.at(i).at(j) == CellState::RABBIT)
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

    mutex::winmutex grid_sync;
    thread::winthread test_thread;

    std::vector<std::vector<CellState>> grid;
    std::vector<std::vector<CellState>> test_thread_grid;

    std::atomic<int> simulationDelayAtomic { 5 };
    std::atomic<double> simulationExecutionTime { 0.0 };

    std::atomic<bool> isTestGridDirty { false };
    std::atomic<bool> isSimulationActiveAtomic { false };

    void UpdateMainThreadGrid(const std::vector<std::vector<CellState>>& updated)
    {
        grid_sync.lock();
        grid = updated;
        grid_sync.unlock();
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
        if (test_thread.is_running())
        {
            test_thread.terminate();
        }

        return false;
    }

    return true;
}
void ImGUILayer::Render()
{
    double tick;
    double end_time;
    double start_time;

    static int threads = 4;
    static int gridSize = 20;

    static std::unordered_map<CellState, ImVec4> cell_colors =
            {
                    { CellState::NONE, ImVec4(0.0f, 0.0f, 0.0f, 1.0f)},
                    { CellState::WOLF, ImVec4(0.49f, 0.49f, 0.49f, 1.0f)},
                    { CellState::RABBIT, ImVec4(1.0f, 0.0f, 1.0f, 1.0f)}
            };

    static int brushSize = 1;
    static CellState currentBrush = CellState::WOLF;

    ImGui::Begin("Wolf v Rabbit");
    start_time = omp_get_wtime();

    ImGui::DragInt("Threads count", &threads, 0.05F, 1, omp_get_max_threads());
    if (DrawButtonConditionally("Update threads count", test_thread.is_running() && threads > 0
            , threads > 0 ? "Better not to change this while test is running" : "Incorrect amount of threads"))
    {
        omp_set_num_threads(threads);
    }

    ImGui::SliderInt("Brush Size", &brushSize, 1, 20);

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

    float cell_spacing = gridSize < 200 ? 1.0F : 0.0F;

    const auto sx = (ImGui::GetContentRegionAvail().x * 0.75F / static_cast<float>(gridSize)) - cell_spacing;
    const auto sy = (ImGui::GetContentRegionAvail().y * 0.75F / static_cast<float>(gridSize)) - cell_spacing;

    float cell_size = std::min(sx, sy);

    float total_cell_size = cell_size + cell_spacing;

    int simulationDelay = simulationDelayAtomic.load();
    ImGui::DragInt("Grid size: ", &gridSize, 0.05F, 1);
    if (DrawButtonConditionally("100x100", gridSize == 100, "Already the selected size"))
    {
        gridSize = 100;
    }
    ImGui::SameLine();

    if (DrawButtonConditionally("200x200", gridSize == 200, "Already the selected size"))
    {
        gridSize = 200;
    }
    ImGui::SameLine();

    if (DrawButtonConditionally("400x400", gridSize == 400, "Already the selected size"))
    {
        gridSize = 400;
    }
    ImGui::SameLine();

    if (DrawButtonConditionally("800x800", gridSize == 800, "Already the selected size"))
    {
        gridSize = 800;
    }

    ImGui::DragInt("Simulation delay: ", &simulationDelay, 0.05F, 1);

    simulationDelayAtomic = std::max(simulationDelay, 0);

    ImVec2 mousePos = ImGui::GetMousePos();
    bool isMouseDown = ImGui::IsMouseDown(0);

    ImVec2 grid_window_size;

    grid_window_size.x = ImGui::GetContentRegionMax().x;
    grid_window_size.y = total_cell_size * static_cast<float>(gridSize);

    if (ImGui::Button("Simulate"))
    {
        isSimulationActiveAtomic = true;
        test_thread.run(
            [=]()
            {
                while(isSimulationActiveAtomic.load())
                {
                    static auto local_grid = grid;

                    if (isTestGridDirty.load())
                    {
                        grid_sync.lock();

                        local_grid = grid;
                        isTestGridDirty = false;

                        grid_sync.unlock();
                    }

                    const auto start_time = omp_get_wtime();
                    Simulate(local_grid);
                    simulationExecutionTime = omp_get_wtime() - start_time;

                    if (simulationDelayAtomic.load() > 0)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(simulationDelayAtomic.load()));
                    }

                     if (!isTestGridDirty.load())
                     {
                         UpdateMainThreadGrid(local_grid);
                     }
                }
            });
    }

    ImGui::SameLine();
    if (DrawButtonConditionally("Stop", !test_thread.is_running(), "Simulation was not started"))
    {
        isSimulationActiveAtomic = false;
    }

    float totalGridWidth = total_cell_size * static_cast<float>(gridSize);
    float remainingWidth = ImGui::GetContentRegionAvail().x - totalGridWidth;

    ImGui::BeginChild("GridChild", grid_window_size, false, ImGuiWindowFlags_HorizontalScrollbar);

    ImVec2 cursorStart = ImGui::GetCursorScreenPos();
    cursorStart.x += remainingWidth * 0.5F;

    if (grid.size() != gridSize)
    {
        isTestGridDirty = true;
        grid.resize(gridSize);
    }

    const float brushSizeAdjustment = cell_size * static_cast<float>(brushSize - 1);

    for (int i = 0; i < grid.size(); i++)
    {
        std::vector<CellState>& row = grid.at(i);

        if (row.size() != gridSize)
        {
            isTestGridDirty = true;
            row.resize(gridSize);
        }

        for (int j = 0; j < row.size(); j++)
        {
            const ImVec2 cellMin(cursorStart.x + j * total_cell_size, cursorStart.y + i * total_cell_size);
            const ImVec2 cellMax(cellMin.x + cell_size, cellMin.y + cell_size);

            const bool isInsideBrush =
                    mousePos.x >= (cellMin.x - brushSizeAdjustment) &&
                    mousePos.x <= (cellMax.x + brushSizeAdjustment) &&
                    mousePos.y >= (cellMin.y - brushSizeAdjustment) &&
                    mousePos.y <= (cellMax.y + brushSizeAdjustment);

            if (isMouseDown && ImGui::IsItemActive() && isInsideBrush)
            {
                isTestGridDirty = true;
                row.at(j) = currentBrush;
            }

            ImVec4 color;
            const CellState cellState = row.at(j);

            auto it = cell_colors.find(cellState);
            if (it == cell_colors.end())
            {
                std::cerr << "Unexpected cell state value: " << static_cast<int>(cellState) << std::endl;
                color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            }
            else
            {
                color = it->second;
            }

            ImGui::GetWindowDrawList()->AddRectFilled(cellMin, cellMax, ImGui::ColorConvertFloat4ToU32(color));
        }
    }

    ImGui::EndChild();

    tick = omp_get_wtick();
    end_time = omp_get_wtime();

    DisplayBoolColored("Is simulation thread running", test_thread.is_running());

    ImGui::Text("Timer precision %lf\n", tick);
    ImGui::Text("Per-Cycle simulation time, in ms %lf\n", simulationExecutionTime.load() * 1000.0);
    ImGui::Text("Render time (including operations), in ms %lf\n", (end_time - start_time) * 1000.0);

    ImGui::End();
}
