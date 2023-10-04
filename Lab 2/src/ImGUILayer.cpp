#include <ImGUILayer.hpp>
#include <core/include/Event.hpp>
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
    using MatrixType = std::vector<std::vector<double>>;

    void DisplayBoolColored(const char* label, bool value)
    {
        ImVec4 color = value ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f);

        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::Text("%s: %s", label, value ? "true" : "false");
        ImGui::PopStyleColor();
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

    m_window = std::make_unique<graphics::Window>("Lab 2 by Retro52", 1280, 720);

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

void RenderMultiplicationWindow()
{
    static MatrixType matrix_a;
    static MatrixType matrix_b;
    static MatrixType matrix_mul_result_non_parallel;
    static MatrixType matrix_mul_result_parallel;

    static int rows_a = 0;
    static int rows_b = 0;

    static int cols_a = 0;
    static int cols_b = 0;

    static double execution_time_parallel = 0.0;
    static double execution_time_non_parallel = 0.0;

    double tick;
    double end_time;
    double start_time;

    static bool can_multiply = false;
    static bool can_terminate_test = false; // we better not terminate the test till pragma omp is running, trust me, I`ve tried

    static retro::thread::winthread test_thread;

    start_time = omp_get_wtime();
    ImGui::Begin("Matrix multiplication");

    static int threads = 4;
    ImGui::DragInt("Threads count", &threads, 0.05F, 0, omp_get_max_threads());
    if (DrawButtonConditionally("Update threads count", test_thread.is_running() && threads > 0
            , threads > 0 ? "Better not to change this while test is running" : "Incorrect amount of threads"))
    {
        omp_set_num_threads(threads);
    }

    // Input for Matrix A size
    ImGui::InputInt("Matrix A Rows", &rows_a);
    ImGui::InputInt("Matrix A Columns", &cols_a);

    if (DrawButtonConditionally("Randomize Matrix A", test_thread.is_running(), "Can`t randomize a matrix while thread test is running"))
    {
        matrix_a = MatrixType(rows_a, std::vector<double>(cols_a, 0.0));
        RandomizeMatrix(matrix_a);
    }

    // Input for Matrix B size
    ImGui::InputInt("Matrix B Rows", &rows_b);
    ImGui::InputInt("Matrix B Columns", &cols_b);

    if (DrawButtonConditionally("Randomize Matrix B", test_thread.is_running(), "Can`t randomize a matrix while thread test is running"))
    {
        matrix_b = MatrixType(rows_b, std::vector<double>(cols_b, 0.0));
        RandomizeMatrix(matrix_b);
    }

    DrawMatrix(matrix_a, "Matrix A: ");
    DrawMatrix(matrix_b, "Matrix B: ");

    DrawMatrix(matrix_mul_result_parallel, "Result of a parallel multiplication: ");
    DrawMatrix(matrix_mul_result_non_parallel, "Result of a non-parallel multiplication: ");

    can_multiply = !matrix_a.empty() && matrix_a.at(0).size() == matrix_b.size();

    if (DrawButtonConditionally("Multiplication test", test_thread.is_running() || !can_multiply,  "Test is already running or A column count != B rows count"))
    {
        test_thread.run(
                [=]()
                {
                    execution_time_parallel = 0.0;
                    execution_time_non_parallel = 0.0;

                    const auto local_rows_a = matrix_a.size();

                    const auto local_cols_a = matrix_a.at(0).size();
                    const auto local_cols_b = matrix_b.at(0).size();

                    matrix_mul_result_parallel = MatrixType (local_rows_a, std::vector<double>(local_cols_b, 0.0));
                    matrix_mul_result_non_parallel = MatrixType (local_rows_a, std::vector<double>(local_cols_b, 0.0));

                    int i, j, k;
                    MatrixType::value_type::value_type sum;
                    const auto parallel_start_time = omp_get_wtime();

                    can_terminate_test = false;
#pragma omp parallel for private(j, k, sum) shared(matrix_mul_result_parallel)
                    for(i = 0; i < local_rows_a; i++)
                    {
                        for(k = 0; k < local_cols_b; k++)
                        {
                            sum = 0;
                            for(j = 0; j < local_cols_a; j++)
                            {
                                sum += matrix_a.at(i).at(j) * matrix_b.at(j).at(k);
                            }

                            matrix_mul_result_parallel.at(i).at(k) = sum;
                        }
                    }
                    can_terminate_test = true;
                    execution_time_parallel = omp_get_wtime() - parallel_start_time;

                    const auto non_parallel_start_time = omp_get_wtime();
                    for(i = 0; i < local_rows_a; i++)
                    {
                        for(k = 0; k < local_cols_b; k++)
                        {
                            sum = 0;
                            for(j = 0; j < local_cols_a; j++)
                            {
                                sum += matrix_a.at(i).at(j) * matrix_b.at(j).at(k);
                            }

                            matrix_mul_result_non_parallel.at(i).at(k) = sum;
                        }
                    }
                    execution_time_non_parallel = omp_get_wtime() - non_parallel_start_time;
                });
    }

    tick = omp_get_wtick();
    end_time = omp_get_wtime();

    DisplayBoolColored("Can multiply: ", can_multiply);
    DisplayBoolColored("Is test thread running", test_thread.is_running());

    if (DrawButtonConditionally("Terminate test", !test_thread.is_running() || !can_terminate_test, "Nothing to terminate or OMP computations are not over"))
    {
        test_thread.terminate();
    }

    ImGui::Text("Timer precision %lf\n", tick);
    ImGui::Text("Execution time parallel, ms %lf\n", execution_time_parallel * 1000.0);
    ImGui::Text("Execution time non-parallel, ms %lf\n", execution_time_non_parallel * 1000.0);
    ImGui::Text("Render time (including operations), in ms %lf\n", (end_time - start_time) * 1000.0);

    ImGui::End();
}

void RenderMatrixRowSumCalculationWindow()
{
    static MatrixType matrix;

    static int rows = 0;
    static int cols = 0;

    double tick;
    double end_time;
    double start_time;

    static double execution_time_parallel = 0.0;
    static double execution_time_non_parallel = 0.0;

    static bool can_terminate_test = false;
    static retro::thread::winthread test_thread;

    static MatrixType sums_result_parallel;
    static MatrixType sums_result_non_parallel;

    static MatrixType::value_type::value_type total_parallel = 0.0;
    static MatrixType::value_type::value_type total_non_parallel = 0.0;

    start_time = omp_get_wtime();
    ImGui::Begin("Row sum calculation");

    ImGui::InputInt("Rows count", &rows);
    ImGui::InputInt("Columns count", &cols);
    if (DrawButtonConditionally("Randomize Matrix", test_thread.is_running(), "Can`t randomize a matrix while thread test is running"))
    {
        matrix = MatrixType(rows, std::vector<double>(cols, 0.0));
        RandomizeMatrix(matrix);
    }
    DrawMatrix(matrix, "Generated matrix: ");

    if (DrawButtonConditionally("Rows addition test test", test_thread.is_running(),  "Test is already running"))
    {
        test_thread.run(
                [&]()
                {
                    sums_result_parallel.clear();
                    sums_result_non_parallel.clear();

                    sums_result_parallel.resize(matrix.size(), std::vector<double>(2, 0.0));
                    sums_result_non_parallel.resize(matrix.size(), std::vector<double>(2, 0.0));

                    total_parallel = 0.0;
                    total_non_parallel = 0.0;

                    execution_time_parallel = 0.0;
                    execution_time_non_parallel = 0.0;

                    int i, j;
                    MatrixType::value_type::value_type sum;

                    can_terminate_test = false;
                    const auto parallel_start_time = omp_get_wtime();

                    auto local_total_parallel = 0.0;
#pragma omp parallel for shared(matrix, sums_result_parallel) private(i, j, sum) reduction (+:local_total_parallel)
                    for (i = 0; i < matrix.size(); i++)
                    {
                        sum = 0;
                        for (j = i; j < matrix.at(i).size(); j++)
                        {
                            sum += matrix.at(i).at(j);
                        }

                        sums_result_parallel.at(i).at(0) = i;
                        sums_result_parallel.at(i).at(1) = sum;

                        local_total_parallel += sum;
                    }

                    total_parallel = local_total_parallel;
                    execution_time_parallel = omp_get_wtime() - parallel_start_time;

                    can_terminate_test = true;
                    const auto non_parallel_start_time = omp_get_wtime();

                    for (i = 0; i < matrix.size(); i++)
                    {
                        sum = 0;
                        for (j = i; j < matrix.at(i).size(); j++)
                        {
                            sum += matrix.at(i).at(j);
                        }

                        total_non_parallel += sum;

                        sums_result_non_parallel.at(i).at(0) = i;
                        sums_result_non_parallel.at(i).at(1) = sum;
                    }
                    execution_time_non_parallel = omp_get_wtime() - non_parallel_start_time;
                });
    }

    tick = omp_get_wtick();
    end_time = omp_get_wtime();

    DisplayBoolColored("Is test thread running", test_thread.is_running());
    if (DrawButtonConditionally("Terminate test", !test_thread.is_running() || !can_terminate_test, "Nothing to terminate or OMP computations are not over"))
    {
        test_thread.terminate();
    }

    DrawMatrix(sums_result_parallel, "Sums per row calculated in parallel");
    ImGui::Text("Total matrix sum calculated in parallel %lf\n", total_parallel);

    DrawMatrix(sums_result_non_parallel,  "Sums per row calculated not in parallel");
    ImGui::Text("Total matrix sum calculated not in parallel %lf\n", total_non_parallel);

    ImGui::Text("Timer precision %lf\n", tick);
    ImGui::Text("Execution time parallel, ms %lf\n", execution_time_parallel * 1000.0);
    ImGui::Text("Execution time non-parallel, ms %lf\n", execution_time_non_parallel * 1000.0);
    ImGui::Text("Render time (including operations), in ms %lf\n", (end_time - start_time) * 1000.0);

    ImGui::End();
}

void ImGUILayer::Render()
{
    RenderMultiplicationWindow();
    RenderMatrixRowSumCalculationWindow();
}
