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

    double ApproximatePi(const int samples)
    {
        int s;
        int counter = 0;
        constexpr double radius = 1.0;

#pragma omp parallel for reduction(+:counter)
        for (s = 0; s < samples; s++)
        {
            auto x = retro::core::random::generate(- radius, radius);
            auto y = retro::core::random::generate(- radius, radius);

            if ((x * x + y * y) < radius)
            {
                counter++;
            }
        }

        return 4.0 * counter / samples;
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

    static double execution_time = 0.0;
    static thread::winthread test_thread;

    static int n = 500;
    static int threads = 4;
    static double result = 0.0;

    struct HistoryEntry
    {
        double exec_time = 0.0;
        double deviation = 0.0;
        double approx_result = 0.0;

        int steps_count = 1;
        int threads_count = 1;
    };

    static std::vector<HistoryEntry> execution_time_history;

    ImGui::Begin("PI approximation");
    start_time = omp_get_wtime();

    ImGui::InputInt("Samples count", &n);
    ImGui::DragInt("Threads count", &threads, 0.05F, 1, omp_get_max_threads());
    if (DrawButtonConditionally("Update threads count", test_thread.is_running() && threads > 0
            , threads > 0 ? "Better not to change this while test is running" : "Incorrect amount of threads"))
    {
        omp_set_num_threads(threads);
    }

    if (DrawButtonConditionally("Run calculations", test_thread.is_running(), "Calculations are already running"))
    {
        test_thread.run(
                [=]()
                {
                    HistoryEntry entry;

                    entry.steps_count = n;
                    entry.threads_count = threads;

                    execution_time = 0.0;
                    const auto execution_start = omp_get_wtime();
                    result = ApproximatePi(n);
                    execution_time = omp_get_wtime() - execution_start;

                    entry.approx_result = result;
                    entry.exec_time = execution_time;
                    entry.deviation = std::abs(result - std::numbers::pi);

                    execution_time_history.emplace_back(entry);
                });
    }

    ImGui::Text("Perfect result: %lf", std::numbers::pi);
    ImGui::Text("Approximation result: %lf", result);

    const auto deviation = std::abs(result - std::numbers::pi);

    const ImVec4 bad_color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
    const ImVec4 good_color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);

    constexpr auto eas_func = linear<double>;

    constexpr auto bad_deviation = 0.1;
    constexpr auto good_deviation = 0.0;

    DrawColorLerp(good_deviation
                  , bad_deviation
                  , deviation
                  , eas_func
                  , good_color
                  , bad_color
                  , ImGui::Text
                  , "Approx. deviation: %lf"
                  , deviation);

    tick = omp_get_wtick();
    end_time = omp_get_wtime();

    DisplayBoolColored("Is test thread running", test_thread.is_running());

    ImGui::Text("Timer precision %lf\n", tick);
    ImGui::Text("Execution time parallel, ms %lf\n", execution_time * 1000.0);
    ImGui::Text("Render time (including operations), in ms %lf\n", (end_time - start_time) * 1000.0);

    if (DrawButtonConditionally("Clear history", execution_time_history.empty(), "History is already as clean as my browser`s one"))
    {
        execution_time_history.clear();
    }

    if (!execution_time_history.empty())
    {
        if (ImGui::BeginTable("Execution Time Table", 5, ImGuiTableFlags_Sortable | ImGuiTableFlags_SizingFixedSame))
        {
            // Table headers
            ImGui::TableSetupColumn("Execution time", ImGuiTableColumnFlags_DefaultSort);
            ImGui::TableSetupColumn("Calculated PI", ImGuiTableColumnFlags_DefaultSort);
            ImGui::TableSetupColumn("Approx. abs deviation", ImGuiTableColumnFlags_DefaultSort);
            ImGui::TableSetupColumn("Threads count", ImGuiTableColumnFlags_NoSort);
            ImGui::TableSetupColumn("Samples count", ImGuiTableColumnFlags_NoSort);

            ImGui::TableHeadersRow();

            // Sort our data if the user clicked on one of the headers
            if (ImGuiTableSortSpecs* sortsSpecs = ImGui::TableGetSortSpecs())
            {
                if (sortsSpecs->SpecsDirty)
                {
                    std::stable_sort(execution_time_history.begin(), execution_time_history.end(), [&sortsSpecs](const HistoryEntry& lhs, const HistoryEntry& rhs)
                    {
                        for (int n = 0; n < sortsSpecs->SpecsCount; n++)
                        {
                            const ImGuiTableColumnSortSpecs* sortSpec = &sortsSpecs->Specs[n];
                            if (sortSpec->ColumnIndex == 0) // Execution Time column
                            {
                                if (sortSpec->SortDirection == ImGuiSortDirection_Ascending)
                                {
                                    return lhs.exec_time < rhs.exec_time;
                                }
                                else
                                {
                                    return lhs.exec_time > rhs.exec_time;
                                }
                            }
                            if (sortSpec->ColumnIndex == 1) // Result column
                            {
                                if (sortSpec->SortDirection == ImGuiSortDirection_Ascending)
                                {
                                    return lhs.approx_result < rhs.approx_result;
                                }
                                else
                                {
                                    return lhs.approx_result > rhs.approx_result;
                                }
                            }
                            if (sortSpec->ColumnIndex == 2) // Deviation column
                            {
                                if (sortSpec->SortDirection == ImGuiSortDirection_Ascending)
                                {
                                    return lhs.deviation < rhs.deviation;
                                }
                                else
                                {
                                    return lhs.deviation > rhs.deviation;
                                }
                            }
                        }
                        return false;
                    });

                    sortsSpecs->SpecsDirty = false;
                }
            }

            // Populate rows
            for (const auto& entry : execution_time_history)
            {
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%.3f ms", entry.exec_time * 1000.0);

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%f", entry.approx_result);

                ImGui::TableSetColumnIndex(2);
                DrawColorLerp(good_deviation
                        , bad_deviation
                        , entry.deviation
                        , eas_func
                        , good_color
                        , bad_color
                        , ImGui::Text
                        , "%lf"
                        , entry.deviation);

                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%d", entry.threads_count);

                ImGui::TableSetColumnIndex(4);
                ImGui::Text("%d", entry.steps_count);
            }

            ImGui::EndTable();
        }
    }

    ImGui::End();
}
