#include "AppUI.h"

struct RollingBuffer
{
  float Span;
  ImVector<ImVec2> Data;
  RollingBuffer()
  {
    Span = 7.0f;
    Data.reserve(100);
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
    ImPlot::SetupAxisLimits(ImAxis_X1, 0, 7, ImGuiCond_Always);
    ImPlot::SetupAxisLimits(ImAxis_Y1, -100, 3500);
    ImPlot::PlotLine("A", &va.Data[0].x, &va.Data[0].y, va.Data.size(), 0, 0, 2 * sizeof(float));
    ImPlot::PlotLine("B", &vb.Data[0].x, &vb.Data[0].y, vb.Data.size(), 0, 0, 2 * sizeof(float));
    ImPlot::PlotLine("C", &vc.Data[0].x, &vc.Data[0].y, vc.Data.size(), 0, 0, 2 * sizeof(float));
    ImPlot::PlotLine("D", &vd.Data[0].x, &vd.Data[0].y, vd.Data.size(), 0, 0, 2 * sizeof(float));
    ImPlot::EndPlot();
  }
}

AppUI::AppUI(GLFWwindow *window, const char *glsl_version) : classifier(), extractor(), keybrd()
{
  AppUI::dataType = 0;
  AppUI::lastSelected = 0;
  AppUI::trainingSamples = 50;
  AppUI::currentPort = 0;
  AppUI::votingCount = 1;
  AppUI::triggerCount = 50;
  AppUI::lastPrediction = -1;
  AppUI::prediction = 0;
  AppUI::act = 0;
  AppUI::sendActive = false;
  std::vector<std::vector<double>> empty;
  AppUI::trainingData = std::vector<std::vector<std::vector<double>>>(5, empty);
  AppUI::triggers = std::vector<int>(5, 0);

  AppUI::finger = 0;

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
  if (!extractor.savedData.size())
  {

    ImGui::Text("Record");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8);
    ImGui::InputInt("Sample/s", &trainingSamples, 10, 1000);
    extractor.saveDataSize = trainingSamples;
    ImGui::SameLine();
    if (ImGui::Button("Record Data") && !extractor.saveData)
      extractor.saveData = true;
    /*
    if (ImGui::Button("Export Recordings") && !extractor.saveData)
      exportRecordings();
    */

    ImGui::SameLine();
    if (ImGui::Button("Train"))
    {
      classifier.setData(trainingData);
      classifier.train();
    }
  }
  else
  {
    ImGui::ProgressBar(float(extractor.savedData.size()) / trainingSamples);
  }

  if (!extractor.saveData && extractor.savedData.size() == trainingSamples)
  {
    trainingData.at(finger).clear();
    trainingData.at(finger) = extractor.savedData;
    extractor.savedData.clear();
    finger = finger < 4 ? finger + 1 : 0;
  }

  if (ImGui::BeginTable("table", 4, ImGuiTableFlags_Borders))
  {
    ImGui::TableNextColumn();
    ImGui::Text("Finger");
    ImGui::TableNextColumn();
    ImGui::Text("Recorder Samples", ImGui::GetContentRegionAvail().x);
    ImGui::TableNextColumn();
    ImGui::Text("Key", ImGui::GetContentRegionAvail().x);
    ImGui::TableNextColumn();
    ImGui::Text("Trigger", ImGui::GetContentRegionAvail().x);
    for (int n = 0; n < 5; n++)
    {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::RadioButton(labels.at(n).c_str(), &finger, n);
      ImGui::TableNextColumn();
      ImGui::Text("%d", trainingData.at(n).size());
      ImGui::TableNextColumn();
      const char *data = reinterpret_cast<const char *>(&n); // convert to a char pointer
      ImGui::Combo(data, &(keybrd.keyMappings[n]),keybrd.keys.data(), keybrd.keys.size());
      ImGui::TableNextColumn();
      ImGui::ProgressBar((float)triggers.at(n) / (float)triggerCount);
      // ImGui::Text("%d/%d", triggers.at(n), triggerCount);
    }
    ImGui::EndTable();
  }
}
void AppUI::connect(const char *port)
{
  if (extractor.start(port))
  {
    std::thread thr(&Extractor::filterDataTask, &extractor);
    AppUI::thread_filter = &thr;
    thr.detach();
  }
}

void AppUI::predictCurrent()
{
  if (classifier.trained)
  {
    if (classifier.accuracy > 0)
    {
      ImGui::Text("Accuracy: %.2f ", classifier.accuracy);
      ImGui::SameLine();
    }

    extractor.extractMutex.lock();
    std::vector<double> combined;
    combined.reserve(extractor.dataRMS.size() + extractor.dataWL.size());
    combined.insert(combined.end(), extractor.dataRMS.begin(), extractor.dataRMS.end());
    combined.insert(combined.end(), extractor.dataWL.begin(), extractor.dataWL.end());

    extractor.extractMutex.unlock();
    int pred = classifier.predict(combined);
    if (pred >= 0 && pred < 5)
    {
      prediction = pred;
    }
    if (lastPrediction != prediction)
    {

      lastPrediction = prediction;
      triggers = std::vector<int>(5, 0);
    }
    else
    {
      triggers.at(prediction)++;
      for (size_t i = 0; i < 5; i++)
      {
        if (triggers.at(i) >= triggerCount)
        {
          act = i;
          triggers = std::vector<int>(5, 0);
          if (sendActive)
            keybrd.sendKey(keybrd.keyMappings[act]);
          break;
        }
      }
    }

    ImGui::SetNextItemWidth(200.0);
    ImGui::Text("Prediction: %s", labels.at(prediction).c_str());
    ImGui::SameLine(200);
    ImGui::Text("P Trigger: %s", labels.at(act).c_str());
    ImGui::SameLine(400);
    ImGui::Text("Trigger Count:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 6);
    ImGui::SliderInt("     ", &triggerCount, 1, 100);
    ImGui::SameLine();
    ImGui::Checkbox("Active", &sendActive);
  }
}
/// @brief Update the contents.
void AppUI::update()
{

  if (extractor.connectionMutex.try_lock())
  {
    if (!extractor.connected)
    {
      updateConnectionTab();
    }
    else
    {
      updateSignalPlotTab();
    }
    extractor.connectionMutex.unlock();
  }
}
void AppUI::updateSignalPlotTab()
{
  float vva = 0.0f;
  float vvb = 0.0f;
  float vvc = 0.0f;
  float vvd = 0.0f;

  ImGui::Begin("Signal Plot", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
  if (ImGui::Button("Close Port"))
  {
    extractor.terminate();
  }

  ImGui::SameLine();
  ImGui::Text("Capturing %d samples per second", extractor.rawRate);

  ImGui::Text("Target Extraction Rate:");
  ImGui::SameLine();
  ImGui::SetNextItemWidth(ImGui::GetFontSize() * 24);
  ImGui::SliderInt(" ", &extractor.targetExtractRate, 1, 500, "%dHz");
  ImGui::SameLine();
  ImGui::Text("Real Rate: %dHz", extractor.extractRate);
  ImGui::Text("Data Type: ");
  ImGui::SameLine();
  ImGui::RadioButton("Raw", &dataType, 0);
  ImGui::SameLine();
  ImGui::RadioButton("RMS", &dataType, 1);
  ImGui::SameLine();
  if (ImGui::Button("Calibrate") && extractor.calibrated)
    extractor.calibrated = false;
  ImGui::SameLine();
  ImGui::RadioButton("WL", &dataType, 2);

  std::vector<double> data;
  bool locked = extractor.extractMutex.try_lock();
  if (locked)
  {
    switch (dataType)
    {
    case 0:
      data = extractor.dataRaw;
      break;
    case 1:
      data = extractor.dataRMS;
      break;
    case 2:
      data = extractor.dataWL;
      break;
    default:
      break;
    }
    vva = (float)data.at(0);
    vvb = (float)data.at(1);
    vvc = (float)data.at(2);
    vvd = (float)data.at(3);
    extractor.extractMutex.unlock();
  }

  RealtimePlots(vva, vvb, vvc, vvd, lastSelected != dataType);
  lastSelected = dataType;

  recordingElement();
  predictCurrent();

  ImGui::End();
}
void AppUI::updateConnectionTab()
{
  ImGui::Begin("Connection", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
  availablePorts.clear();
  std::vector<std::string> portList = listPorts();
  for (int i = 0; i < portList.size(); ++i)
    availablePorts.push_back(portList[i].c_str());
  ImGui::Text("Serial Ports");
  ImGui::SameLine();
  ImGui::Combo(" ", &currentPort, availablePorts.data(), availablePorts.size());
  ImGui::SameLine();
  if (ImGui::Button("Open Port" ) && availablePorts.size())
  {
    extractor.connectionMutex.unlock();
    connect(availablePorts[currentPort]);
    extractor.connectionMutex.lock();
  }

  ImGui::End();
}
AppUI::~AppUI()
{
}

// Closes the thread and calls the UserInterface::shutdown().
void AppUI::shutdown()
{
  // filter.terminate();
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
