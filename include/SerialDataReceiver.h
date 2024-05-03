#pragma once
#include <chrono>
#include <cstdint>
#include <errno.h>
#include <fcntl.h> // File control definitions
#include <iomanip>
#include <iostream>
#include <mutex>
#include <optional>
#include <sstream>
#include <string.h>
#include <termios.h> // POSIX terminal control definitions
#include <thread>
#include <unistd.h> // UNIX standard function definitions
#include <vector>

class SerialDataReceiver
{
public:
  std::mutex m;
  std::vector<uint16_t> data;
  SerialDataReceiver( speed_t baudRate);
  ~SerialDataReceiver();
  void setPort(const char *port);
  bool openPort();
  bool closePort();
  std::vector<uint16_t> receiveData();
  int restartDevice();
  int getSampleRate() const;
  int getErrorCount() const;

private:
  speed_t baudRate;
  int serial_fd_;
  int sampleRate = 0;
  int errorCount = 0;
  const char *port;
  struct termios tty_;
  std::vector<uint16_t> extractData(char buffer[]);
};