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
  bool *connected;
  int *status;
  int finger;
  int dataType;
  int lastSelected;
  int trainingSamples;
  std::vector<std::vector<std::vector<double>>> trainingData;
  std::vector<std::string> labels;

  SerialDataReceiver *sdr;
  EMGFilter *filter;
  std::thread *thread_filter;
  std::vector<const char *> availablePorts;
  std::vector<std::string> listPorts();
  Classifier classifier;
  void recordingElement();
  void predictCurrent();
  void exportRecordings();

public:
  AppUI(GLFWwindow *window, const char *glsl_version, SerialDataReceiver *sdr, EMGFilter *filter, int *status, bool *connected);
  ~AppUI();
  virtual void update() override;
  virtual void shutdown() override;
};
