#pragma once
#include <cstdint>
#include <errno.h>
#include <fcntl.h> // File control definitions
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string.h>
#include <termios.h> // POSIX terminal control definitions
#include <vector>
#include <unistd.h> // UNIX standard function definitions
#include <chrono>
#include <thread>
#include <mutex>

class SerialDataReceiver
{
public:
  std::mutex m;
  std::vector<uint16_t> data;
  SerialDataReceiver(const char* port, int baudRate, int* status);
  ~SerialDataReceiver();

  int openPort();
  void closePort();
  void receiveData();
  int getSampleRate() const;
  int getErrorCount() const;

private:
  int baudRate;
  int* status;
  int serial_fd_;
  int sampleRate = 0;
  int errorCount = 0;
  const char* port;
  struct termios tty_;
  std::vector<uint16_t> extractData(char buffer[]);
};