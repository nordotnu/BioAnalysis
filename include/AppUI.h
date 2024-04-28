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

class AppUI : public UserInterface
{
private:

  bool* connected;
  int* status;
  int finger;
  bool showFiltered;

  SerialDataReceiver *sdr;
  EMGFilter *filter;
  std::thread* thread_filter;
  std::vector<const char *> availablePorts;

  std::vector<std::string> listPorts();

public:
  AppUI(GLFWwindow *window, const char *glsl_version, SerialDataReceiver *sdr, EMGFilter* filter, int* status, bool* connected);
  ~AppUI();
  virtual void update() override;
  virtual void shutdown() override;
};
