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
  void clear()
  {
    Data.shrink(0);
  }
};

void RealtimePlots(float valueA, float valueB, float valueC, float valueD, bool reset = false)
{
  static RollingBuffer va, vb, vc, vd;
  static float t = 0;
  if (reset)
  {
    va.clear();
    vb.clear();
    vc.clear();
    vd.clear();
    t = 0;
  }
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

AppUI::AppUI(GLFWwindow *window, const char *glsl_version, SerialDataReceiver *sdr, EMGFilter *filter, int *status, bool *connected) : classifier()
{
  AppUI::sdr = sdr;
  AppUI::filter = filter;
  AppUI::connected = connected;
  AppUI::status = status;

  AppUI::dataType = 0;
  AppUI::lastSelected = 0;
  AppUI::trainingSamples = 100;
  std::vector<std::vector<double>> empty;
  AppUI::trainingData = std::vector<std::vector<std::vector<double>>>(5, empty);

  AppUI::finger = 0;
  std::thread thr(&EMGFilter::filterDataTask, filter);
  AppUI::thread_filter = &thr;
  AppUI::thread_filter->detach();

  AppUI::labels = {"Resting", "Index", "Middle", "Ring", "Pinky"};

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

void AppUI::recordingElement()
{
  if (!filter->savedData.size())
  {

    ImGui::Text("Record");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8);
    ImGui::InputInt("Sample/s", &trainingSamples, 10, 1000);
    filter->saveDataSize = trainingSamples;
    ImGui::SameLine();
    if (ImGui::Button("Record Data") && !filter->saveData)
      filter->saveData = true;
    ImGui::SameLine();
    if (ImGui::Button("Train"))
    {
      classifier.setData(trainingData);
      classifier.train();
    }
  }
  else
  {
    ImGui::ProgressBar(float(filter->savedData.size()) / trainingSamples);
  }

  if (!filter->saveData && filter->savedData.size() == trainingSamples)
  {
    trainingData.at(finger).clear();
    trainingData.at(finger) = filter->savedData;
    filter->savedData.clear();
    finger = finger < 4 ? finger + 1 : 0;
  }

  if (ImGui::BeginTable("table", 2, ImGuiTableFlags_Borders))
  {
    ImGui::TableNextColumn();
    ImGui::Text("Finger");
    ImGui::TableNextColumn();
    ImGui::Text("Recorder Samples", ImGui::GetContentRegionAvail().x);
    for (int n = 0; n < 5; n++)
    {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::RadioButton(labels.at(n).c_str(), &finger, n);
      ImGui::TableNextColumn();
      ImGui::Text("%d", trainingData.at(n).size());
    }
    ImGui::EndTable();
  }
}
void AppUI::predictCurrent()
{
  if (classifier.trained)
  {
    ImGui::Text("Accuracy: %.2f ", classifier.accuracy);
    std::vector<double> current;
    bool locked = filter->filterMutex.try_lock();
    if (locked)
    {
      current.reserve(filter->dataRMS.size() + filter->dataWL.size());
      current.insert(current.end(), filter->dataRMS.begin(), filter->dataRMS.end());
      current.insert(current.end(), filter->dataWL.begin(), filter->dataWL.end());
      filter->filterMutex.unlock();
    }
    int prediction = classifier.predict(current);
    if (prediction > -1 && prediction < 5)
    {
      ImGui::SameLine();
      ImGui::Text("%s", labels.at(prediction).c_str());
    }
  }
}
/// @brief Update the contents.
void AppUI::update()
{

  float vva = 0.0f;
  float vvb = 0.0f;
  float vvc = 0.0f;
  float vvd = 0.0f;

  ImGui::Begin("Connection", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
  if (!*connected && *status != 0)
  {
    availablePorts.clear();
    std::vector<std::string> portList = listPorts();
    for (int i = 0; i < portList.size(); ++i)
      availablePorts.push_back(portList[i].c_str());
    int currentPort = 0;
    ImGui::Text("Serial Ports");
    ImGui::SameLine();
    ImGui::Combo(" ", &currentPort, availablePorts.data(), availablePorts.size());
    ImGui::SameLine();
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

    ImGui::Begin("Signal Plot", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

    ImGui::Text("Target Filter Rate:");
    ImGui::SameLine();
    ImGui::SliderInt(" ", &filter->targetFilterRate, 5, 500, "%dHz");
    ImGui::SameLine();
    ImGui::Text("Real Rate: %dHz", filter->filterRate);
    ImGui::Text("Data Type: ");
    ImGui::SameLine();
    ImGui::RadioButton("Raw", &dataType, 0);
    ImGui::SameLine();
    ImGui::RadioButton("RMS", &dataType, 1);
    ImGui::SameLine();
    if (ImGui::Button("Calibrate") && filter->calibrated)
      filter->calibrated = false;
    ImGui::SameLine();
    ImGui::RadioButton("WL", &dataType, 2);

    std::vector<double> data;
    bool locked = filter->filterMutex.try_lock();
    if (locked)
    {
      switch (dataType)
      {
      case 0:
        data = filter->dataRaw;
        break;
      case 1:
        data = filter->dataRMS;
        break;
      case 2:
        data = filter->dataWL;
        break;
      default:
        break;
      }
      vva = (float)data.at(0);
      vvb = (float)data.at(1);
      vvc = (float)data.at(2);
      vvd = (float)data.at(3);
      filter->filterMutex.unlock();
    }

    RealtimePlots(vva, vvb, vvc, vvd, lastSelected != dataType);
    lastSelected = dataType;

    recordingElement();
    predictCurrent();

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

void AppUI::exportRecordings()
{
  std::ofstream outputFile("data.csv", std::ios::app);
  if (outputFile.is_open())
  {
    for (size_t i = 0; i < trainingData.size(); i++)
    {
      std::vector<std::vector<double>> fingerData = trainingData.at(i);
      for (size_t j = 0; j < fingerData.size(); j++)
      {
        outputFile << i << ",";
        for (size_t k = 0; k < 8; k++)
        {
          outputFile << (int)fingerData[j][k];
          if (k != 7)
            outputFile << ",";
        }
        outputFile << "\n";
      }
    }
    outputFile.close();
  }
  else
  {
    std::cerr << "Error opening file\n";
  }
}
