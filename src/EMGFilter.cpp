#include "EMGFilter.h"

EMGFilter::EMGFilter(SerialDataReceiver *sdr, bool *connected, int dataCount, int targetFilterRate, int targetRawRate)
{
  EMGFilter::sdr = sdr;
  EMGFilter::connected = connected;
  EMGFilter::dataCount = dataCount;
  EMGFilter::targetFilterRate = targetFilterRate;
  EMGFilter::targetRawRate = targetRawRate;
  EMGFilter::terminated = false;
  EMGFilter::calibrated = false;
}

EMGFilter::~EMGFilter()
{
}

/// @brief Terminate the thread (if the class is running on a seperate thread).
void EMGFilter::terminate()
{
  EMGFilter::terminated = true;
}

/// @brief Subroutine to fetch the data from the serialDataReceiver and filter the data.
/// @return 0 if interupted or finished.
int EMGFilter::filterDataTask()
{
  std::vector<double> temp;
  for (size_t i = 0; i < dataCount; i++)
  {
    temp.push_back(0);
    data.push_back(0);
  }
  int currentFilterRate = 0;
  int currentRawSamples = 0;
  int count = 0;
  auto startFilter = std::chrono::high_resolution_clock::now();
  auto startRaw = std::chrono::high_resolution_clock::now();
  auto startRate = std::chrono::high_resolution_clock::now();
  while (!terminated)
  {
    if (!*connected)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      continue;
    }

    auto currentTime = std::chrono::high_resolution_clock::now();
    auto elapsedFilter = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - startFilter);
    auto elapsedRaw = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startRaw);
    auto elapsedRate = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startRate);

    if (elapsedRate.count() >= 1000)
    {
      startRate = currentTime;
      filterRate = currentFilterRate;
      rawRate = currentRawSamples;
      currentFilterRate = 0;
      currentRawSamples = 0;
    }
    if (elapsedRaw.count() >= (1000 / targetRawRate) && *connected)
    {
      startRaw = currentTime;
      std::vector<uint16_t> raw = sdr->receiveData();

      if (raw.size() == 4)
      {
        for (size_t i = 0; i < dataCount; i++)
        {

          temp.at(i) += pow(raw.at(i), 2);
        }
        count++;
        currentRawSamples++;
      }
    }

    int tt = targetFilterRate;

    if (rawRate > 0 && tt > rawRate)
    {
      tt = rawRate;
    }

    if (elapsedFilter.count() >= (1e6 / tt))
    {
      filterMutex.lock();
      currentFilterRate++;
      startFilter = currentTime;
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
        data.clear();
        for (size_t i = 0; i < dataCount; i++)
        {
          double value = sqrt(temp.at(i) / count) / calibrationData.at(i);
          data.push_back(value);
          temp.at(i) = 0;
        }
      }
      count = 0;
      if (saveData)
      {
        savedData.push_back(data);
        if (savedData.size() >= 500)
        {
          saveData = false;
        }
      }
      filterMutex.unlock();
    }
  }
  return 0;
}
