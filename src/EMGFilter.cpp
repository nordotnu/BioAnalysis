#include "EMGFilter.h"

EMGFilter::EMGFilter(std::vector<uint16_t> *rawData, std::mutex *rawMutex, int *status, int dataCount, int samplesPerSec)
{
  EMGFilter::rawData = rawData;
  EMGFilter::rawMutex = rawMutex;
  EMGFilter::status = status;
  EMGFilter::dataCount = dataCount;
  EMGFilter::samplesPerSec = samplesPerSec;
}

EMGFilter::~EMGFilter()
{
}

void EMGFilter::filterData()
{
  std::vector<double> temp;
  for (size_t i = 0; i < dataCount; i++)
  {
    temp.push_back(0);
    data.push_back(0);
  }

  int samples = 0;
  int count = 0;
  auto start_time = std::chrono::high_resolution_clock::now();
  auto rate_start = std::chrono::high_resolution_clock::now();

  printf("status=%d, dataCount=%d", *status, dataCount);
  while (*status == 0)
  {
    auto rate_current = std::chrono::high_resolution_clock::now();
    auto rate_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(rate_current - rate_start);

    auto current_time = std::chrono::high_resolution_clock::now();
    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time);
    if (rate_elapsed.count() >= 1000)
    {
      rate_start = rate_current;
      EMGFilter::filterRate = samples;
      samples = 0;
    }
    if (elapsed_time.count() >= 2)
    {
      const std::lock_guard<std::mutex> lock(*rawMutex);

      if (rawData->size() == 4)
      {
        for (size_t i = 0; i < dataCount; i++)
        {

          temp.at(i) += pow(rawData->at(i), 2);
        }

        count++;
      }
    }
    if (count >= (35 * samplesPerSec))
    {
      const std::lock_guard<std::mutex> lock(filterMutex);

      samples++;
      data.clear();
      for (size_t i = 0; i < dataCount; i++)
      {
        data.push_back(sqrt(temp.at(i) / count));
        temp.at(i) = 0;
      }
      count = 0;
    }
  }
}

int EMGFilter::getFilterRate() const
{
  return filterRate;
}