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

    const int GRID_SIZE = 20;
    enum class CellState
    {
        NONE,
        WOLF,
        RABBIT
    };

    std::unordered_map<CellState, ImVec4> cell_colors_float =
    {
            {CellState::NONE, ImVec4(0.0f, 0.0f, 0.0f, 1.0f)},
            {CellState::WOLF, ImVec4(0.49f, 0.49f, 0.49f, 1.0f)},
            {CellState::RABBIT, ImVec4(1.0f, 0.0f, 1.0f, 1.0f)}
    };

    CellState currentBrush = CellState::WOLF;
    CellState grid[GRID_SIZE][GRID_SIZE] = {};

    void RenderBrushSelector()
    {
        CellState newBrush = currentBrush;
        if (currentBrush == CellState::NONE)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, cell_colors_float[CellState::NONE]);
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
            ImGui::PushStyleColor(ImGuiCol_Button, cell_colors_float[CellState::WOLF]);
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
            ImGui::PushStyleColor(ImGuiCol_Button, cell_colors_float[CellState::RABBIT]);
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
        ImGui::ColorEdit3("NONE Color", (float*)&cell_colors_float[CellState::NONE]);
        ImGui::ColorEdit3("WOLF Color", (float*)&cell_colors_float[CellState::WOLF]);
        ImGui::ColorEdit3("RABBIT Color", (float*)&cell_colors_float[CellState::RABBIT]);
    }

    void RenderGrid()
    {
        RenderBrushSelector();

        if (ImGui::Button("Clear"))
        {

        }

        ImVec2 originalSpacing = ImGui::GetStyle().ItemSpacing;
        ImGui::GetStyle().ItemSpacing = ImVec2(2.0f, 2.0f);

        for (auto & row : grid)
        {
            for (int j = 0; j < GRID_SIZE; j++)
            {
                auto color = cell_colors_float.at(row[j]);

                ImGui::PushStyleColor(ImGuiCol_Button, color);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
                ImGui::PushID(&row[j]);

                if (ImGui::Button("", ImVec2(40, 40)) ||
                    (ImGui::IsItemHovered() && ImGui::IsMouseDown(0)))
                {
                    row[j] = currentBrush;
                }

                ImGui::PopStyleColor(2);
                ImGui::PopID();

                if (j < GRID_SIZE - 1)
                {
                    ImGui::SameLine();
                }
            }
        }

        ImGui::GetStyle().ItemSpacing = originalSpacing;
    }

//    void UpdateGrid
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

    m_window->GetDirectXSwapChain()->Present(1, 0);

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

void ImGUILayer::Render()
{
    ImGui::Begin("Wolf v Rabbit");

    RenderGrid();

    ImGui::End();
}
