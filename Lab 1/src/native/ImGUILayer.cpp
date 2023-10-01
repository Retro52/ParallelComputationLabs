#include <core/Timer.hpp>
#include <native/ImGUILayer.hpp>

#include <array>
#include <iostream>
#include <algorithm>

#include <thread>
#include <imgui.h>

#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_dx11.h>

using namespace retro;

namespace
{
    constexpr const char * async_default = "retro+";

    int sleep_duration = 500;
    bool track_timer = false;
    bool apply_lock_guard = false;

    mutex::winmutex async_mutex;
    std::string async_test = async_default;

    const std::array<const char*, 5> priority_names =
    {
            "Low",
            "Below Normal",
            "Normal",
            "Above Normal",
            "High"
    };

    void async_invoke_func(size_t append)
    {
        auto local_apply_guard = apply_lock_guard;

        if (local_apply_guard)
        {
            async_mutex.lock();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_duration));

        std::cerr << "Append!" << std::endl;
        async_test += std::to_string(append) + "-";

        if (local_apply_guard)
        {
            async_mutex.unlock();
        }
    }

    void DisplayBoolColored(const char* label, bool value)
    {
        ImVec4 color = value ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f);

        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::Text("%s: %s", label, value ? "true" : "false");
        ImGui::PopStyleColor();
    }
}

void ImGUILayer::OnAttach()
{
    m_window = std::make_unique<graphics::Window>();

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
    m_window->PollEvents();

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
        std::cerr << "Error! Exception details: " << e.what() << std::endl;
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
    std::cerr << "Layer removed: " << std::endl;
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
    static std::vector<double> exec_time_history;

    ImGui::Begin("Threads");

    ImGui::SliderInt("Sleep duration: ", &sleep_duration, 100, 2000);

    ImGui::TextColored({ 1.0F, 0.5F, 0.5F, 1.0F }, "Not so atomic string: %s", async_test.c_str());

    if (ImGui::Button("Reset string to default"))
    {
        async_test = async_default;
    }

    ImGui::Checkbox("Wrap resource in mutex?", &apply_lock_guard);

    if (ImGui::Button("Add new thread"))
    {
        m_threads.emplace_back(async_invoke_func, m_threads.size());
    }

    bool all_finished = true;
    for (int32_t i = 0; i < m_threads.size(); i++)
    {
        all_finished &= m_threads.at(i).is_finished();

        ImGui::PushID(&m_threads.at(i));

        ImGui::Text("Thread %d", i);
        ImGui::SameLine();

        if (ImGui::Button("Run"))
        {
            m_threads.at(i).run();
        }
        ImGui::SameLine();

        if (ImGui::Button("Join"))
        {
            m_threads.at(i).join();
        }
        ImGui::SameLine();

        if (ImGui::Button("Pause"))
        {
            m_threads.at(i).pause();
        }
        ImGui::SameLine();

        if (ImGui::Button("Resume"))
        {
            m_threads.at(i).resume();
        }
        ImGui::SameLine();

        if (ImGui::Button("Terminate"))
        {
            m_threads.at(i).terminate();
        }
        ImGui::SameLine();

        if (ImGui::Button("Remove"))
        {
            m_threads.erase(m_threads.cbegin() + i);

            ImGui::PopID();
            break;
        }
        ImGui::SameLine();

        ImGui::SetNextItemWidth(250.0F);
        auto cur_priority = static_cast<int>(m_threads.at(i).get_priority());
        if (ImGui::Combo("Priority", &cur_priority, priority_names.data(), static_cast<int>(priority_names.size())))
        {
            m_threads.at(i).set_priority(static_cast<retro::thread::priority>(cur_priority));
        }
        ImGui::SameLine();

        DisplayBoolColored("Is paused", m_threads.at(i).is_paused());
        ImGui::SameLine();

        DisplayBoolColored("Is running", m_threads.at(i).is_running());
        ImGui::SameLine();

        DisplayBoolColored("Is finished", m_threads.at(i).is_finished());
        ImGui::SameLine();

        ImGui::PopID();
        ImGui::NewLine();
    }

    if (all_finished && track_timer)
    {
        track_timer = false;
        m_all_threads_run_timer.Stop();

        exec_time_history.push_back((m_all_threads_run_timer.Tick<std::chrono::microseconds>() / 1000.0).count());
    }

    if (!m_threads.empty())
    {
        if (ImGui::Button("Run all"))
        {
            track_timer = true;
            m_all_threads_run_timer.Run();
            std::for_each(m_threads.begin(), m_threads.end(), [](auto& thread) { thread.run(); });
        }
        ImGui::SameLine();

        if (ImGui::Button("Pause all"))
        {
            std::for_each(m_threads.begin(), m_threads.end(), [](auto& thread) { thread.pause(); });
        }
        ImGui::SameLine();

        if (ImGui::Button("Resume all"))
        {
            std::for_each(m_threads.begin(), m_threads.end(), [](auto& thread) { thread.resume(); });
        }
        ImGui::SameLine();

        if (ImGui::Button("Terminate all"))
        {
            std::for_each(m_threads.begin(), m_threads.end(), [](auto& thread) { thread.terminate(); });
        }
    }

    const auto exec_time = m_all_threads_run_timer.Tick<std::chrono::microseconds>() / 1000.0;

    ImGui::Text("Execution time: %f", exec_time.count());

    if (!exec_time_history.empty())
    {
        if (ImGui::CollapsingHeader("Runtime history"))
        {
            const ImVec4 text_color = { 1.0F, 0.5F, 0.5F, 1.0F };
            for (const auto& time : exec_time_history)
            {
                ImGui::TextColored(text_color, "Exec time: %f", time);
            }

            if (ImGui::Button("Clear"))
            {
                exec_time_history.clear();
            }
        }
    }

    ImGui::End();
}
