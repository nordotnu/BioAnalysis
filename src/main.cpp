#include "../include/SerialDataReceiver.h"


int main()
{
  int status = 0;
  SerialDataReceiver sdr("/dev/ttyUSB0",123, &status);
  sdr.openPort();
  std::thread thread_obj(&SerialDataReceiver::receiveData, &sdr);

  auto startTime = std::chrono::high_resolution_clock::now();
  while (status == 0)
  {
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime);
    if (elapsedTime.count() >= 2)
    {
      startTime = currentTime;
      sdr.m.lock();
      if (sdr.data.size() == 4)
      {
        printf("A: %04u, B: %4u, C: %04u, D: %04u (R=%d), ERROR=%d, status=%d\n", sdr.data.at(0), sdr.data.at(1), sdr.data.at(2), sdr.data.at(3), sdr.getSampleRate(), sdr.getErrorCount(), status);
        std::cout << "\033[1A"
                  << "\033[K";
      }
      sdr.m.unlock();
    }
  }
  sdr.closePort();
}
