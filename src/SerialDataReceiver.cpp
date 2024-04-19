
#include "SerialDataReceiver.h"

/// @brief Constructor
/// @param port Path to the serial port (e.g. "/dev/ttyUSB0").
/// @param baudRate Baud Rate.
/// @param status A pointer to the status int, outputs 0 when an error occures.
SerialDataReceiver::SerialDataReceiver(const char *port, speed_t baudRate, int *status)
{
  SerialDataReceiver::port = port;
  SerialDataReceiver::baudRate = baudRate;
  SerialDataReceiver::status = status;
}

SerialDataReceiver::~SerialDataReceiver()
{
  closePort();
}

/// @brief Opens the Serial port for reading.
/// @return Result - 0 if success.
int SerialDataReceiver::openPort()
{
  SerialDataReceiver::serial_fd_ = open(port, O_RDWR); // Adjust the device path if needed

  // Error checking
  if (SerialDataReceiver::serial_fd_ == -1)
  {
    std::cerr << "Error opening serial port: " << strerror(errno) << std::endl;
    *status = -1;
    return -1;
  }
  struct termios tty;

  // Get current settings
  if (tcgetattr(SerialDataReceiver::serial_fd_, &tty) != 0)
  {
    std::cerr << "Error from tcgetattr: " << strerror(errno) << std::endl;
    *status = -1;
    return -1;
  }

  // Configure settings
  cfsetospeed(&tty, baudRate); // Set baud rate
  cfsetispeed(&tty, baudRate);

  // Other common settings if needed:
  // tty.c_cflag &= ~PARENB;        // No parity
  // tty.c_cflag &= ~CSTOPB; // One stop bit
  // tty.c_cflag &= ~CSIZE;         // Clear character size bits
  // tty.c_cflag |= CS8; // 8 bits per byte
  // tty.c_cflag &= ~CRTSCTS;       // Disable hardware flow control
  // tty.c_lflag &= ~ICANON; // Disable canonical mode (read line-by-line)
  // tty.c_cc[VTIME] = 1;   // Read timeout of 1 second
  //tty.c_cc[VMIN] = 0; // Non-blocking read
  

  // Apply settings
  if (tcsetattr(SerialDataReceiver::serial_fd_, TCSANOW, &tty) != 0)
  {
    std::cerr << "Error from tcsetattr: " << strerror(errno) << std::endl;
    *status = -1;
    return -1;
  }
  return 0;
}

void SerialDataReceiver::closePort()
{
  *status = -1;
  int closed = close(SerialDataReceiver::serial_fd_);
  printf("Closed = %d", closed);
}

/// @brief Subroutine task to read the data from the serial port. openPort() should be called before.
void SerialDataReceiver::receiveData()
{
  if (*status != 0)
  {
    return;
  }
  int byte_count = 512;
  char buffer[byte_count];
  u_long total_samples = 1;
  int samples = 0;
  int last_values = 0;

  auto start_time = std::chrono::high_resolution_clock::now();
  auto rate_start = std::chrono::high_resolution_clock::now();
  while (*status == 0)
  {

    auto rate_current = std::chrono::high_resolution_clock::now();
    auto rate_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(rate_current - rate_start);

    auto current_time = std::chrono::high_resolution_clock::now();
    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time);
    if (rate_elapsed.count() >= 1000)
    {
      rate_start = rate_current;
      total_samples += samples;
      SerialDataReceiver::sampleRate = samples;
      samples = 0;
    }
    if (elapsed_time.count() >= 2)
    {
      // m.lock();
      const std::lock_guard<std::mutex> lock(m);
      start_time = current_time; // Reset the timer

      int bytes_read = read(SerialDataReceiver::serial_fd_, buffer, sizeof(buffer));

      if (bytes_read < 0)
      {
        std::cerr << "Error reading from serial port: " << strerror(errno) << std::endl;
        *status = -1;
        return;
      }

      if (buffer[0] != 'S')
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 100 - 200));
        
      }
      else
      {
        std::vector<uint16_t> numbers = extractData(buffer);
        if (numbers.size() == 4)
        {
          int current_values = numbers.at(0) + numbers.at(1) + numbers.at(2) + numbers.at(3);
          data.clear();
          for (int i = 0; i < 4; i++)
          {
            SerialDataReceiver::data.push_back(numbers.at(i));
          }
          if (current_values != last_values)
          {
            samples++;
            last_values = current_values;
          }
        }
        else
        {
          SerialDataReceiver::errorCount++;
        }
      }
      // m.unlock();
    }
  }
  // closePort();
}

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
