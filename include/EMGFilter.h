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
  std::vector<double> data;
  int filterRate;
  int rawRate;
  EMGFilter(SerialDataReceiver *sdr, int *status, int dataCount = 4, int targetFilterRate = 100, int targetRawRate = 500);
  ~EMGFilter();

  int filterDataTask();

private:
  SerialDataReceiver *sdr;
  int dataCount;
  int *status;
  int targetFilterRate;
  int targetRawRate;
};