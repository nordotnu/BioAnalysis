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

class Extractor
{
public:
  std::mutex extractMutex;
  std::mutex connectionMutex;
  std::vector<double> dataRaw;
  std::vector<double> dataRMS;
  std::vector<double> dataWL;
  std::vector<double> calibrationData;
  std::vector<std::vector<double>> savedData;
  std::vector<std::vector<double>> buffer;
  bool calibrated;
  bool saveData;
  int targetExtractRate;
  int extractRate;
  int rawRate;
  int saveDataSize;
  bool connected;
  int bufferSize;

  Extractor(int dataCount = 4, int targetExtractRate = 100, int targetRawRate = 500, int bufferSize = 9);
  ~Extractor();

  bool start(const char *port);
  int filterDataTask();
  void terminate();

private:
  bool terminated;
  SerialDataReceiver sdr;
  int dataCount;
  int targetRawRate;
};