
#include "SerialDataReceiver.h"
#include <cmath>

/// @brief Constructor
/// @param port Path to the serial port (e.g. "/dev/ttyUSB0").
/// @param baudRate Baud Rate.
/// @param status A pointer to the status int, outputs 0 when an error occures.
SerialDataReceiver::SerialDataReceiver(speed_t baudRate)
{
  SerialDataReceiver::baudRate = baudRate;
  filterA.init(SAMPLE_FREQ_500HZ, NOTCH_FREQ_50HZ, true, true, true);
  filterB.init(SAMPLE_FREQ_500HZ, NOTCH_FREQ_50HZ, true, true, true);
  filterC.init(SAMPLE_FREQ_500HZ, NOTCH_FREQ_50HZ, true, true, true);
  filterD.init(SAMPLE_FREQ_500HZ, NOTCH_FREQ_50HZ, true, true, true);
}

SerialDataReceiver::~SerialDataReceiver()
{
  // closePort();
}

void SerialDataReceiver::setPort(const char *port)
{
  SerialDataReceiver::port = port;
}

/// @brief Opens the Serial port for reading.
/// @return Result - 0 if success.
bool SerialDataReceiver::openPort()
{
  SerialDataReceiver::serial_fd_ = open(port, O_RDWR); // Adjust the device path if needed

  // Error checking
  if (SerialDataReceiver::serial_fd_ == -1)
  {
    std::cerr << "Error opening serial port: " << strerror(errno) << std::endl;
    return false;
  }
  struct termios tty;

  // Get current settings
  if (tcgetattr(SerialDataReceiver::serial_fd_, &tty) != 0)
  {
    std::cerr << "Error from tcgetattr: " << strerror(errno) << std::endl;
    return false;
  }

  // Configure settings
  cfsetospeed(&tty, baudRate); // Set baud rate
  cfsetispeed(&tty, baudRate);

  // Other common settings if needed:
  // tty.c_cflag &= ~PARENB;        // No parity
  // tty.c_cflag &= ~CSTOPB; // One stop bit
  // tty.c_cflag &= ~CSIZE;         // Clear character size bits
  // tty.c_cflag |= CS8; // 8 bits per byte
  tty.c_cflag &= ~CRTSCTS; // Disable hardware flow control
                           // tty.c_lflag &= ~ICANON; // Disable canonical mode (read line-by-line)
  tty.c_cc[VTIME] = 1;     // Read timeout of 1 second
                           // tty.c_cc[VMIN] = 0; // Non-blocking read

  // Apply settings
  if (tcsetattr(SerialDataReceiver::serial_fd_, TCSANOW, &tty) != 0)
  {
    std::cerr << "Error from tcsetattr: " << strerror(errno) << std::endl;
    return false;
  }
  return true;
}

bool SerialDataReceiver::closePort()
{
 return close(SerialDataReceiver::serial_fd_) == 0;
}

/// @brief Subroutine task to read the data from the serial port. openPort() should be called before.
std::vector<uint16_t> SerialDataReceiver::receiveData()
{
  std::vector<uint16_t> numbers;
  int byte_count = 512;
  char buffer[byte_count];
  int bytes_read = read(SerialDataReceiver::serial_fd_, buffer, sizeof(buffer));

  if (bytes_read < 0)
  {
    std::cerr << "Error reading from serial port: " << strerror(errno) << std::endl;
    return numbers;
  }
  if (buffer[0] == 'S')
  {
    numbers = extractData(buffer);
    if (numbers.size() == 4)
    {
      /*
      numbers[0] = sqrt(filterA.update(numbers[0])) * 1000;
      numbers[1] = sqrt(filterB.update(numbers[1])) * 1000;
      numbers[2] = sqrt(filterC.update(numbers[2])) * 1000;
      numbers[3] = sqrt(filterD.update(numbers[3])) * 1000;

      */
      return numbers;
    }
  }
  return numbers;
}

/// @brief Sends restart command to the device.
/// @return Whether the command has been sent.
int SerialDataReceiver::restartDevice()
{
  const char ack = 'R';
  int bytes_write = write(SerialDataReceiver::serial_fd_, &ack, 1);
  if (bytes_write < 0)
  {
    std::cerr << "Error writing to serial port: " << strerror(errno) << std::endl;
    return -1;
  }
  return 0;
}

/// @brief Gets the current sampling rate per second.
/// @return Samples per second.
int SerialDataReceiver::getSampleRate() const
{
  return sampleRate;
}

/// @brief Gets the error count in the reading of data.
/// @return number of errors.
int SerialDataReceiver::getErrorCount() const
{
  return errorCount;
}

std::vector<uint16_t> SerialDataReceiver::extractData(char buffer[])
{

  std::vector<uint16_t> numbers;
  auto str = std::string(buffer).substr(1, strlen(buffer) - 2);

  std::stringstream ss(str);
  std::string temp;
  int num;

  while (std::getline(ss, temp, ','))
  {
    if (std::stringstream(temp) >> num)
    {
      numbers.push_back(num);
    }
  }
  int error = 0;
  for (auto &element : numbers)
  {
    if (element > 4095)
    {
      error++;
    }
  }
  if (numbers.size() != 4 || error != 0)
  {
    numbers.clear();
  }
  return numbers;
}
