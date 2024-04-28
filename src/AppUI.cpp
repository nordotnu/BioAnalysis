
#include "AppUI.h"

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

AppUI::AppUI(GLFWwindow *window, const char *glsl_version, SerialDataReceiver *sdr, EMGFilter *filter, int *status, bool *connected)
{
  AppUI::sdr = sdr;
  AppUI::filter = filter;
  AppUI::connected = connected;
  AppUI::status = status;

  finger = 0;
  showFiltered = true;
  std::thread thr(&EMGFilter::filterDataTask, filter);
  AppUI::thread_filter = &thr;
  thread_filter->detach();

  UserInterface::init(window, glsl_version);
}

std::vector<std::string> AppUI::listPorts()
{
  std::vector<std::string> ports;
  std::string path = "/dev";
  for (const auto &entry : std::filesystem::directory_iterator(path))
  {
    if (entry.path().filename().string().substr(0, 6).contains("ttyUSB"))
    {
      ports.push_back(entry.path().string());
    }
  }
  return ports;
}

/// @brief Update the contents.
void AppUI::update()
{

  float vva = 0.0f;
  float vvb = 0.0f;
  float vvc = 0.0f;
  float vvd = 0.0f;

  if (*connected)
  {
    bool locked = filter->filterMutex.try_lock();
    if (locked)
    {
      vva = (float)filter->data.at(0);
      vvb = (float)filter->data.at(1);
      vvc = (float)filter->data.at(2);
      vvd = (float)filter->data.at(3);
      filter->filterMutex.unlock();
    }
  }

  ImGui::Begin("Connection", nullptr, ImGuiWindowFlags_NoTitleBar);
  if (!*connected && *status != 0)
  {
    availablePorts.clear();
    std::vector<std::string> portList = listPorts();
    for (int i = 0; i < portList.size(); ++i)
      availablePorts.push_back(portList[i].c_str());
    
    int currentPort = 0;
    ImGui::ListBox("Serial Ports", &currentPort, availablePorts.data(), availablePorts.size());

    if (ImGui::Button("Open Port") && availablePorts.size())
    {
      sdr->setPort(availablePorts[currentPort]);
      if (sdr->openPort() == 0)
      {
        *connected = true;
      }
    }
  }
  else if (ImGui::Button("Close Port") && *status == 0)
  {
    *connected = false;
    sdr->closePort();
  }

  ImGui::End();

  if (*connected)
  {

    ImGui::Begin("Signal Plot", nullptr, ImGuiWindowFlags_NoTitleBar);
    ImGui::SliderInt("Target Filter Rate", &filter->targetFilterRate, 5, 500);

    RealtimePlots(vva, vvb, vvc, vvd);
    ImGui::Text("Sampling Rate: %d Hz, Filtering Rate: %d Hz", filter->rawRate, filter->filterRate);

    ImGui::SliderInt("Finger", &finger, 0, 4);
    if (ImGui::Button("Calibrate") && filter->calibrated)
    {
      filter->calibrated = false;
    }
    if (ImGui::Button("Save Data") && !filter->saveData)
    {
      filter->saveData = true;
    }
    if (!filter->saveData && filter->savedData.size() == 500)
    {
      std::ofstream outputFile("data.csv");
      if (outputFile.is_open())
      {

        for (size_t i = 0; i < filter->savedData.size(); i++)
        {
          outputFile << finger << ",";
          for (size_t j = 0; j < 4; j++)
          {
            outputFile << (int)filter->savedData[i][j];
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

      filter->savedData.clear();
    }
    ImGui::End();
  }
}

AppUI::~AppUI()
{
}

// Closes the thread and calls the UserInterface::shutdown().
void AppUI::shutdown()
{
  sdr->closePort();
  *connected = false;
  *status = -1;
  filter->terminate();

  UserInterface::shutdown();
}
