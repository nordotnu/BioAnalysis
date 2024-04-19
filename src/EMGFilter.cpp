#include "EMGFilter.h"

EMGFilter::EMGFilter(SerialDataReceiver *sdr, int *status, int dataCount, int targetFilterRate, int targetRawRate)
{
  EMGFilter::sdr = sdr;
  EMGFilter::status = status;
  EMGFilter::dataCount = dataCount;
  EMGFilter::targetFilterRate = targetFilterRate;
  EMGFilter::targetRawRate = targetRawRate;
}

EMGFilter::~EMGFilter()
{
  *status = -1;
}

int EMGFilter::filterDataTask()
{
  std::vector<double> temp;
  for (size_t i = 0; i < dataCount; i++)
  {
    temp.push_back(0);
    data.push_back(0);
  }
  int currentFilterRate = 0;
  int currentRawSamples= 0;
  int count = 0;
  auto startFilter= std::chrono::high_resolution_clock::now();
  auto startRaw = std::chrono::high_resolution_clock::now();
  auto startRate = std::chrono::high_resolution_clock::now();
  while (*status == 0)
  {
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto elapsedFilter = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startFilter);
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
    if (elapsedRaw.count() >= (1000/targetRawRate))
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
    if (elapsedFilter.count() >= (1000/ targetFilterRate))
    {
      currentFilterRate++;
      startFilter = currentTime;
      const std::lock_guard<std::mutex> lock(filterMutex);
      data.clear();
      for (size_t i = 0; i < dataCount; i++)
      {
        data.push_back(sqrt(temp.at(i) / count));
        temp.at(i) = 0;
      }
        //printf("A: %d, B: %d, C: %d, D: %d, Raw Sample Rate:%d, Filter Rate:%d\n", (int)data.at(0), (int)data.at(1), (int)data.at(2), (int)data.at(3), rawRate, filterRate);
      count = 0;
    }

  }
  printf("STATUS NOT 0\n");
  return 1;
}
