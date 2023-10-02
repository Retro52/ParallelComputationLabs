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
    using MatrixType = std::vector<std::vector<ValueType>>;

    std::vector<ValueType> Solve(MatrixType matrix)
    {
        ValueType tmp;
        int n = static_cast<int>(matrix.size());

        if (n == 0)
        {
            return { };
        }

        std::vector<ValueType> xx(n, 0.0);

        for (int i = 0; i < n; i++)
        {
            tmp = matrix[i][i];
            for (int j = n; j >= i; j--)
            {
                matrix[i][j] /= tmp;
            }

            int j;
            int k;
#pragma omp parallel for private (j, k, tmp)
            for (j = i + 1; j < n; j++)
            {
                tmp = matrix[j][i];
                for (k = n; k >= i; k--)
                {
                    matrix[j][k] -= tmp * matrix[i][k];
                }
            }
        }

        xx[n - 1] = matrix[n - 1][n];
        for (int i = n - 2; i >= 0; i--)
        {
            int j;
            xx[i] = matrix[i][n];

#pragma omp for private (j)
            for (j = i + 1; j < n; j++)
            {
                xx[i] -= matrix[i][j] * xx[j];
            }
        }

        return xx;
    }

    void DrawMatrix(const MatrixType& matrix, const std::string& label)
    {
        constexpr size_t rows_max = 8;
        constexpr size_t max_cols = 8;

        if (!matrix.empty() && !matrix.at(0).empty())
        {
            ImGui::Text("%s", label.c_str());

            if (ImGui::BeginTable(reinterpret_cast<const char *>(&matrix), static_cast<int>(std::min(matrix.at(0).size(), max_cols + 1)), ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedSame | ImGuiTableFlags_NoHostExtendX))
            {
                size_t rows_count = matrix.size();
                size_t cols_count = matrix.at(0).size();

                for (size_t i = 0; i < rows_count; ++i)
                {
                    if (i < rows_max / 2 || i >= rows_count - (rows_max / 2) || rows_count <= rows_max)
                    {
                        ImGui::TableNextRow();

                        for (size_t j = 0; j < cols_count; ++j)
                        {
                            if (j < max_cols / 2 || j >= cols_count - (max_cols / 2) || cols_count <= max_cols)
                            {
                                ImGui::TableNextColumn();
                                ImGui::Text("%5.3lf", matrix.at(i).at(j));
                            }
                            else if (j == max_cols / 2)
                            {
                                ImGui::TableNextColumn();
                                ImGui::Text("... (%llu)", matrix.at(0).size() - max_cols);
                            }
                        }
                    }
                    else if (i == rows_max / 2)
                    {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::Text("... (%llu)", matrix.size() - rows_max);
                        ImGui::TableSetColumnIndex(static_cast<int>(std::min(matrix.at(0).size(), max_cols + 1)) - 1);
                    }
                }
                ImGui::EndTable();
            }
        }
    }

    void RandomizeMatrix(MatrixType& matrix)
    {
        retro::core::ScopeTimer _("Matrix randomize");
        int i, j;
#pragma omp parallel for private(i, j) shared(matrix)
        for (i = 0; i < matrix.size(); i++)
        {
            for (j = 0; j < matrix.at(i).size(); j++)
            {
                matrix.at(i).at(j) = core::random::generate<MatrixType::value_type::value_type>(-100, 100);
            }
        }
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

    static int n = 5;
    static int threads = 4;
    static MatrixType matrix;
    static MatrixType result (1);

    using HistoryEntry = std::pair<int, double>;
    static std::vector<HistoryEntry> execution_time_history;

    ImGui::Begin("Gauss method");
    start_time = omp_get_wtime();

    ImGui::InputInt("Matrix size", &n);

    if (DrawButtonConditionally("Randomize matrix", test_thread.is_running(), "Test is running"))
    {
        matrix = MatrixType (n, MatrixType::value_type(n + 1, 0));
        RandomizeMatrix(matrix);
    }
    DrawMatrix(matrix, "Matrix");

    ImGui::DragInt("Threads count", &threads, 0.05, 0, omp_get_max_threads());
    if (DrawButtonConditionally("Update threads count", test_thread.is_running() && threads > 0
            , threads > 0 ? "Better not to change this while test is running" : "Incorrect amount of threads"))
    {
        omp_set_num_threads(threads);
    }

    if (DrawButtonConditionally("Run calculations", test_thread.is_running() || matrix.empty()
                                , matrix.empty() ? "Matrix is empty" : "Calculations are already running"))
    {
        test_thread.run(
                [=]()
                {
                    execution_time = 0.0;
                    const auto execution_start = omp_get_wtime();
                    result[0] = Solve(matrix);
                    execution_time = omp_get_wtime() - execution_start;

                    execution_time_history.emplace_back(threads, execution_time);
                });
    }


    DrawMatrix(result, "Result");

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
        if (ImGui::BeginTable("Execution Time Table", 2, ImGuiTableFlags_Sortable | ImGuiTableFlags_SizingFixedSame))
        {
            // Table headers
            ImGui::TableSetupColumn("Number of Threads", ImGuiTableColumnFlags_NoSort);
            ImGui::TableSetupColumn("Execution Time (ms)", ImGuiTableColumnFlags_DefaultSort);
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
                            if (sortSpec->ColumnIndex == 1) // Execution Time column
                            {
                                bool result = lhs.second < rhs.second;
                                return sortSpec->SortDirection == ImGuiSortDirection_Ascending == result;
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
                ImGui::Text("%d", entry.first); // Number of threads

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%.2f ms", entry.second * 1000.0); // Execution time in milliseconds
            }

            ImGui::EndTable();
        }
    }

    ImGui::End();
}
