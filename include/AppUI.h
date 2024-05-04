#pragma once
#include "UserInterface.h"
#include "SerialDataReceiver.h"
#include "EMGFilter.h"
#include <filesystem>
#include "implot.h"
#include "math.h"
#include "string"
#include <fstream>
#include <iostream>
#include <vector>
#include <Classifier.h>

class AppUI : public UserInterface
{
private:
  int finger;
  int dataType;
  int lastSelected;
  int trainingSamples;
  int currentPort;
  int votingCount;
  int triggerCount;
  int lastPrediction;
  int act;


  std::vector<int> triggers;
  std::vector<std::vector<std::vector<double>>> trainingData;
  std::vector<std::string> labels;


  EMGFilter filter;
  std::thread *thread_filter;
  std::vector<const char *> availablePorts;
  std::vector<std::string> listPorts();
  Classifier classifier;
  void recordingElement();
  void predictCurrent();
  void updateConnectionTab();
  void updateSignalPlotTab();
  void exportRecordings();
  void connect(const char *port);

public:
  AppUI(GLFWwindow *window, const char *glsl_version);
  ~AppUI();
  virtual void update() override;
  virtual void shutdown() override;
};
