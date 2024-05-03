#pragma once
#include <SerialDataReceiver.h>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string.h>
#include <thread>
#include <vector>

class EMGFilter
{
public:
  std::mutex filterMutex;
  std::mutex connectionMutex;
  std::vector<double> dataRaw;
  std::vector<double> dataRMS;
  std::vector<double> dataWL;
  std::vector<double> calibrationData;
  std::vector<std::vector<double>> savedData;
  bool calibrated;
  bool saveData;
  int targetFilterRate;
  int filterRate;
  int rawRate;
  int saveDataSize;
  bool connected;

  EMGFilter(int dataCount = 4, int targetFilterRate = 100, int targetRawRate = 500);
  ~EMGFilter();

  bool start(const char *port);
  int filterDataTask();
  void terminate();

private:
  bool terminated;
  SerialDataReceiver sdr;
  int dataCount;
  int targetRawRate;
};