#pragma once
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
  EMGFilter(std::vector<uint16_t> *rawData, std::mutex *rawMutex, int *status, int dataCount = 4, int samplesPerSec = 100);
  ~EMGFilter();

  void filterData();
  int getFilterRate() const;

private:
  std::vector<uint16_t> *rawData;
  std::mutex *rawMutex;
  int dataCount;
  int *status;
  int filterRate;
  int samplesPerSec;
};