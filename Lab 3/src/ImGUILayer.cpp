#include <ImGUILayer.hpp>
#include <core/include/Random.hpp>

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
    using ValueType = double;
    using FuncType = std::function<ValueType(ValueType)>;

    ValueType Func(ValueType x)
    {
        return (1 + x) / (std::sqrt(2 * x));
    }

    ValueType IntegrateParallel(const FuncType f, ValueType a, ValueType b, int n)
    {
        ValueType result = 0.0;
        ValueType dx = (b - a) / n;

#pragma omp parallel firstprivate(a, dx, f)
        {
#pragma omp for reduction(+:result)
            for(int i = 0; i <= n; i++)
            {
                ValueType x = a + i * dx;
                if (i == 0 || i == n)
                {
                    result += f(x) / 2.0;
                }
                else
                {
                    result += f(x);
                }
            }
        }

        return result * dx;
    }

    double IntegrateNonParallel(const FuncType& f, ValueType a, ValueType b, int n)
    {
        ValueType result = 0.0;
        ValueType dx = (b - a) / n;

        for(int i = 0; i <= n; i++)
        {
            ValueType x = a + i * dx;
            if (i == 0 || i == n)
            {
                result += f(x) / 2.0;
            }
            else
            {
                result += f(x);
            }
        }

        return result * dx;
    }

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
}

void ImGUILayer::OnAttach()
{
    omp_set_nested(1);

    m_window = std::make_unique<graphics::Window>("Lab 3 by Retro52", 1280, 720);

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
    const auto& io = ImGui::GetIO();
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
    double tick;
    double end_time;
    double start_time;

    static ValueType a = 1.0;
    static ValueType b = 4.0;

    static ValueType result_parallel = 0.0;
    static ValueType result_non_parallel = 0.0;

    static int num_of_steps = 1000;
    static thread::winthread test_thread;

    static double execution_time_parallel = 0.0;
    static double execution_time_non_parallel = 0.0;

    ImGui::Begin("Integration");
    start_time = omp_get_wtime();

    ImGui::InputInt("Number of steps", &num_of_steps, 1);

    ImGui::InputDouble("Integration segment, a: ", &a, 0.5);
    ImGui::InputDouble("Integration segment, b: ", &b, 0.5);

    static int threads = 4;
    ImGui::DragInt("Threads count", &threads, 0.05F, 0, omp_get_max_threads());
    if (DrawButtonConditionally("Update threads count", test_thread.is_running() && threads > 0
            , threads > 0 ? "Better not to change this while test is running" : "Incorrect amount of threads"))
    {
        omp_set_num_threads(threads);
    }

    if(DrawButtonConditionally("Run calculations", test_thread.is_running(), "Calculations are already running"))
    {
        test_thread.run(
            [=]()
            {
                execution_time_parallel = 0.0;
                execution_time_non_parallel = 0.0;

                const auto parallel_start = omp_get_wtime();
                result_parallel = IntegrateParallel(Func, a, b, num_of_steps);
                execution_time_parallel = omp_get_wtime() - parallel_start;

                const auto non_parallel_start = omp_get_wtime();
                result_non_parallel = IntegrateNonParallel(Func, a, b, num_of_steps);
                execution_time_non_parallel = omp_get_wtime() - non_parallel_start;
            });
    }

    ImGui::Text("Integration result parallel: %lf", result_parallel);
    ImGui::Text("Integration result non-parallel: %lf", result_non_parallel);

    tick = omp_get_wtick();
    end_time = omp_get_wtime();

    DisplayBoolColored("Is test thread running", test_thread.is_running());

    ImGui::Text("Timer precision %lf\n", tick);

    ImGui::Text("Execution time parallel, ms %lf\n", execution_time_parallel * 1000.0);
    ImGui::Text("Execution time non-parallel, ms %lf\n", execution_time_non_parallel * 1000.0);

    ImGui::Text("Render time (including operations), in ms %lf\n", (end_time - start_time) * 1000.0);

    ImGui::End();
}
