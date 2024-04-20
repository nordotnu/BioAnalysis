#include "EMGFilter.h"
#include "SerialDataReceiver.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"
#include "math.h"
#include "string"
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include <filesystem>
#include <fstream>
#include <iostream>

static void glfw_error_callback(int error, const char *description)
{
  fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// utility structure for realtime plot
struct RollingBuffer
{
  float Span;
  ImVector<ImVec2> Data;
  RollingBuffer()
  {
    Span = 10.0f;
    Data.reserve(1000);
  }
  void AddPoint(float x, float y)
  {
    float xmod = fmodf(x, Span);
    if (!Data.empty() && xmod < Data.back().x)
      Data.shrink(0);
    Data.push_back(ImVec2(xmod, y));
  }
};

void RealtimePlots(float valueA, float valueB, float valueC, float valueD)
{

  static RollingBuffer va, vb, vc, vd;
  static float t = 0;
  t += ImGui::GetIO().DeltaTime;
  va.AddPoint(t, valueA);
  vb.AddPoint(t, valueB);
  vc.AddPoint(t, valueC);
  vd.AddPoint(t, valueD);

  static ImPlotAxisFlags flags = ImPlotAxisFlags_NoTickLabels;

  if (ImPlot::BeginPlot("sEMG", ImVec2(-1, 400)))
  {
    ImPlot::SetupAxes(nullptr, nullptr, flags, flags);
    ImPlot::SetupAxisLimits(ImAxis_X1, 0, 10, ImGuiCond_Always);
    ImPlot::SetupAxisLimits(ImAxis_Y1, -100, 3500);
    ImPlot::PlotLine("A", &va.Data[0].x, &va.Data[0].y, va.Data.size(), 0, 0, 2 * sizeof(float));
    ImPlot::PlotLine("B", &vb.Data[0].x, &vb.Data[0].y, vb.Data.size(), 0, 0, 2 * sizeof(float));
    ImPlot::PlotLine("C", &vc.Data[0].x, &vc.Data[0].y, vc.Data.size(), 0, 0, 2 * sizeof(float));
    ImPlot::PlotLine("D", &vd.Data[0].x, &vd.Data[0].y, vd.Data.size(), 0, 0, 2 * sizeof(float));
    ImPlot::EndPlot();
  }
}

std::vector<std::string> listPorts()
{
  std::vector<std::string> ports;
  std::string path = "/dev";
  for (const auto &entry : std::filesystem::directory_iterator(path))
  {
    if (entry.path().filename().string().substr(0, 6).contains("ttyUSB"))
    {

      std::cout << "Port:" << entry.path().string() << std::endl;
      ports.push_back(entry.path().string());
    }
  }
  return ports;
}

int main()
{

  std::vector<std::string> portList = listPorts();
  std::vector<const char *> availablePorts;
  for (int i = 0; i < portList.size(); ++i)
    availablePorts.push_back(portList[i].c_str());

  bool connected = false;
  int status = -1;
  int finger = 0;

  SerialDataReceiver sdr(B921600, &status);

  EMGFilter filter(&sdr, &connected);

  std::thread thread_filter(&EMGFilter::filterDataTask, &filter, 12);
  thread_filter.detach();

  // Imgui
  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit())
    return 1;

  const char *glsl_version = "#version 130";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

  GLFWwindow *window = glfwCreateWindow(1280, 720, "BioAnalysis", nullptr, nullptr);
  if (window == nullptr)
    return 1;
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // Enable vsync

  ImGui::CreateContext();
  ImPlot::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.Fonts->AddFontFromFileTTF("inter.ttf", 20.0f);
  // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking
  // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Enable Multi-Viewport / Platform Windows
  //  Setup Dear ImGui style
  ImGui::StyleColorsClassic();

  // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
  ImGuiStyle &style = ImGui::GetStyle();
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
  {
    style.WindowRounding = 4.5f;
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
  }

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);

  bool showFiltered = true;

  while (!glfwWindowShouldClose(window))
  {
    glfwPollEvents();

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    float vva = 0.0f;
    float vvb = 0.0f;
    float vvc = 0.0f;
    float vvd = 0.0f;

    if (connected)
    {

      bool locked = filter.filterMutex.try_lock();
      if (locked)
      {
        vva = (float)filter.data.at(0);
        vvb = (float)filter.data.at(1);
        vvc = (float)filter.data.at(2);
        vvd = (float)filter.data.at(3);
        filter.filterMutex.unlock();
      }
    }

    ImGui::Begin("Connection", nullptr, ImGuiWindowFlags_NoTitleBar);
    int currentPort = 0;
    ImGui::ListBox("Serial Ports", &currentPort, availablePorts.data(), availablePorts.size());
    if (!connected && status != 0)
    {

      if (ImGui::Button("Open Port"))
      {
        sdr.setPort(availablePorts[currentPort]);
        if (sdr.openPort() == 0)
        {
          connected = true;
        }
      }
    }
    else if (ImGui::Button("Close Port") && status == 0)
    {
      connected = false;
      sdr.closePort();
    }

    ImGui::End();

    if (connected)
    {

      ImGui::Begin("Signal Plot", nullptr, ImGuiWindowFlags_NoTitleBar);
      ImGui::SliderInt("Target Filter Rate", &filter.targetFilterRate, 5, 500);

      RealtimePlots(vva, vvb, vvc, vvd);
      ImGui::Text("Sampling Rate: %d Hz, Filtering Rate: %d Hz", filter.rawRate, filter.filterRate);

      ImGui::SliderInt("Finger", &finger, 0, 4);
      if (ImGui::Button("Calibrate") && filter.calibrated)
      {
        filter.calibrated = false;
      }
      if (ImGui::Button("Save Data") && !filter.saveData)
      {
        filter.saveData = true;
      }
      if (!filter.saveData && filter.savedData.size() == 500)
      {
        std::ofstream outputFile("data.csv");
        if (outputFile.is_open())
        {

          for (size_t i = 0; i < filter.savedData.size(); i++)
          {
            outputFile << finger << ",";
            for (size_t j = 0; j < 4; j++)
            {
              outputFile << (int)filter.savedData[i][j];
              if (j != 3)
              {
                outputFile << ",";
              }
            }
            outputFile << "\n";
          }
          outputFile.close();
        }
        else
        {
          std::cerr << "Error opening file\n";
        }

        filter.savedData.clear();
      }
      ImGui::End();
    }

    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Update and Render additional Platform Windows
    // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
    //  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
      GLFWwindow *backup_current_context = glfwGetCurrentContext();
      ImGui::UpdatePlatformWindows();
      ImGui::RenderPlatformWindowsDefault();
      glfwMakeContextCurrent(backup_current_context);
    }

    glfwSwapBuffers(window);
  }
  sdr.closePort();
  connected = false;
  status = -1;
  filter.terminate();
  // thread_filter.join();

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImPlot::DestroyContext();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  glfwTerminate();

  std::terminate();

  return 0;
}
