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
  EMGFilter(SerialDataReceiver *sdr, bool *connected, int dataCount = 4, int targetFilterRate = 100, int targetRawRate = 500);
  ~EMGFilter();

  int filterDataTask();
  void terminate();

private:
  SerialDataReceiver *sdr;
  bool terminated;
  bool *connected;
  int dataCount;
  int targetRawRate;
};