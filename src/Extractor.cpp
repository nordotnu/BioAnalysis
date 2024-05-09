#include "Extractor.h"

Extractor::Extractor(int dataCount, int targetExtractRate, int targetRawRate) : sdr(B921600)
{
  Extractor::dataCount = dataCount;
  Extractor::targetExtractRate = targetExtractRate;
  Extractor::targetRawRate = targetRawRate;
  Extractor::terminated = true;
  Extractor::calibrated = false;
  Extractor::saveData = false;
  Extractor::connected = false;

  extractRate = 0;
  rawRate = 0;
  

  dataRMS = std::vector<double>(dataCount, 0);
  dataRaw = std::vector<double>(dataCount, 0);
  dataWL = std::vector<double>(dataCount, 0);
}

Extractor::~Extractor()
{
}

/// @brief Terminate the thread (if the class is running on a seperate thread).
void Extractor::terminate()
{
  Extractor::terminated = true;
}

bool Extractor::start(const char *port)
{
  const std::lock_guard<std::mutex> lock(connectionMutex);
  sdr.setPort(port);
  if(sdr.openPort()) {
    connected = true;
    terminated = false;
  }
  return connected;
}

/// @brief Subroutine to fetch the data from the serialDataReceiver and extractor the data.
/// @return 0 if interupted or finished.
int Extractor::filterDataTask()
{

  std::vector<double> temp = std::vector<double>(dataCount, 0);

  std::vector<std::vector<uint16_t>> raws;
  for (size_t i = 0; i < dataCount; i++)
  {
    temp.push_back(0);
    dataRMS.push_back(0);
    dataWL.push_back(0);
  }
  int currentFilterRate = 0;
  int currentRawSamples = 0;
  int count = 0;
  auto startFilter = std::chrono::high_resolution_clock::now();
  auto startWave = std::chrono::high_resolution_clock::now();
  auto startRaw = std::chrono::high_resolution_clock::now();
  auto startRate = std::chrono::high_resolution_clock::now();
  while (!terminated)
  {
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto elapsedFilter = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - startFilter);
    auto elapsedWave = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - startWave);
    auto elapsedRaw = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startRaw);
    auto elapsedRate = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startRate);

    // Calculate the real sample rate.
    if (elapsedRate.count() >= 1000)
    {
      startRate = currentTime;
      extractRate = currentFilterRate;
      rawRate = currentRawSamples;
      currentFilterRate = 0;
      currentRawSamples = 0;
    }
    // Read the raw signal.
    if (elapsedRaw.count() >= (1000 / targetRawRate))
    {
      startRaw = currentTime;
      std::vector<uint16_t> raw = sdr.receiveData();

      if (raw.size() == 4)
      {
        raws.push_back(raw);
        for (size_t i = 0; i < dataCount; i++)
        {
          temp.at(i) += pow(raw.at(i), 2);
          dataRaw.at(i) = raw.at(i);
        }
        count++;
        currentRawSamples++;
      }
    }
    // Target extractor rate after limiting it to the max raw sample rate.
    int tt = targetExtractRate;

    if (rawRate > 0 && tt > rawRate)
    {
      tt = rawRate;
    }

    if (elapsedWave.count() >= (1e6 / tt))
    {
      startWave = currentTime;
      std::vector<double> waveLength = std::vector<double>(dataCount, 0);
      for (size_t i = 0; i < raws.size(); i++)
      {
        for (size_t j = 0; j < dataCount; j++)
        {
          if (i + 1 < raws.size())
            waveLength.at(j) += abs(raws.at(i + 1).at(j) - raws.at(i).at(j));
        }
      }
      for (size_t i = 0; i < dataCount; i++)
      {
        if (raws.size() > 0)
          waveLength.at(i) = waveLength.at(i) / raws.size();
      }

      raws.clear();

      extractMutex.lock();
      dataWL = waveLength;
      extractMutex.unlock();
      // printf("A:%f, B:%f, C:%f, D:%f\n", waveLength.at(0), waveLength.at(1), waveLength.at(2), waveLength.at(3));
    }

    // Apply root mean square.
    if (elapsedFilter.count() >= (1e6 / tt))
    {
      extractMutex.lock();
      currentFilterRate++;
      startFilter = currentTime;
      // Calculate the base RMS.
      if (!calibrated)
      {
        calibrated = true;
        calibrationData.clear();
        for (size_t i = 0; i < dataCount; i++)
        {
          double value = sqrt(temp.at(i) / count);
          calibrationData.push_back(value);
          temp.at(i) = 0;
          calibrated = calibrated && (value > 0);
        }
      }
      else
      {
        dataRMS.clear();
        for (size_t i = 0; i < dataCount; i++)
        {
          double value = sqrt(temp.at(i) / count) / calibrationData.at(i);
          dataRMS.push_back(value * 100);
          temp.at(i) = 0;
        }
      }
      count = 0;
      if (saveData)
      {
        std::vector<double> combined;
        combined.reserve(dataRMS.size() + dataWL.size());
        combined.insert(combined.end(), dataRMS.begin(), dataRMS.end());
        combined.insert(combined.end(), dataWL.begin(), dataWL.end());
        savedData.push_back(combined);
        if (savedData.size() >= saveDataSize)
        {
          saveData = false;
        }
      }
      extractMutex.unlock();
    }
  }
  if (sdr.closePort())
  {
    const std::lock_guard<std::mutex> lock(connectionMutex);
    connected = false;
  } else {
    printf("Failed to close the port!\n");
  }
  return 0;
}
