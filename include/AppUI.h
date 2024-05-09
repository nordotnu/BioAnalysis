#pragma once
#include "UserInterface.h"
#include "SerialDataReceiver.h"
#include "Extractor.h"
#include <filesystem>
#include "implot.h"
#include "math.h"
#include "string"
#include <fstream>
#include <iostream>
#include <vector>
#include <Classifier.h>
#include "Keyboard.h"


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
  int prediction;
  int act;
  bool sendActive;

  std::vector<int> triggers;
  std::vector<std::vector<std::vector<double>>> trainingData;
  std::vector<std::string> labels;


  Extractor extractor;
  std::thread *thread_filter;
  std::vector<const char *> availablePorts;
  std::vector<std::string> listPorts();
  Classifier classifier;
  Keyboard keybrd;
  void recordingElement();
  void predictCurrent();
  void updateConnectionTab();
  void updateSignalPlotTab();
  void exportRecordings();
  void connect(const char *port);
  void sendKey(int key);

public:
  AppUI(GLFWwindow *window, const char *glsl_version);
  ~AppUI();
  virtual void update() override;
  virtual void shutdown() override;
};
