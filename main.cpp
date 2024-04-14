#include <chrono>
#include <cstdint>
#include <cstring>
#include <errno.h> // For handling errors (errno)
#include <fcntl.h> // File control definitions
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string.h>
#include <string>
#include <termios.h> // POSIX terminal control definitions
#include <thread>
#include <tuple>
#include <unistd.h> // UNIX standard function definitions
#include <vector>

int main()
{
  int serial_fd = open("/dev/ttyUSB0", O_RDWR); // Adjust the device path if needed

  // Error checking
  if (serial_fd == -1)
  {
    std::cerr << "Error opening serial port: " << strerror(errno) << std::endl;
    return -1;
  }
  struct termios tty;

  // Get current settings
  if (tcgetattr(serial_fd, &tty) != 0)
  {
    std::cerr << "Error from tcgetattr: " << strerror(errno) << std::endl;
    return -1;
  }

  // Configure settings
  cfsetospeed(&tty, B921600); // Set baud rate
  cfsetispeed(&tty, B921600);

  // Other common settings if needed:
  // tty.c_cflag &= ~PARENB;        // No parity
  // tty.c_cflag &= ~CSTOPB; // One stop bit
  // tty.c_cflag &= ~CSIZE;         // Clear character size bits
  // tty.c_cflag |= CS8; // 8 bits per byte
  // tty.c_cflag &= ~CRTSCTS;       // Disable hardware flow control
  // tty.c_lflag &= ~ICANON; // Disable canonical mode (read line-by-line)
  // tty.c_cc[VTIME] = 1;   // Read timeout of 1 second
  // tty.c_cc[VMIN] = 0; // Non-blocking read

  // Apply settings
  if (tcsetattr(serial_fd, TCSANOW, &tty) != 0)
  {
    std::cerr << "Error from tcsetattr: " << strerror(errno) << std::endl;
    return -1;
  }
  int byte_count = 512;
  char buffer[byte_count];
  char ack[] = {'A'};
  bool sync = true;
  // Current sample rate (Unique)
  int sample_rate = 0;
  u_long total_samples = 1;
  int error_count = 0;
  int samples = 0;
  int last_values = 0;

  auto start_time = std::chrono::high_resolution_clock::now();
  auto rate_start = std::chrono::high_resolution_clock::now();
  for (;;)
  {

    auto rate_current = std::chrono::high_resolution_clock::now();
    auto rate_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(rate_current - rate_start);

    auto current_time = std::chrono::high_resolution_clock::now();
    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time);
    if (rate_elapsed.count() >= 1000)
    {
      rate_start = rate_current;
      total_samples += samples;
      sample_rate = samples;
      samples = 0;
    }
    if (elapsed_time.count() >= 1)
    {
      start_time = current_time; // Reset the timer

      int bytes_read = read(serial_fd, buffer, sizeof(buffer));
      if (bytes_read < 0)
      {
        std::cerr << "Error reading from serial port: " << strerror(errno) << std::endl;
        return -1;
      }

      // Force synchronize the serial data with forcing the device to flush the serial buffer.
      if (buffer[0] != 'S')
      {
        sync = false;
        std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 10 - 20));
        int bytes_read = read(serial_fd, buffer, sizeof(buffer));
      }
      else
      {
        sync = true;
        auto str = std::string(buffer).substr(1, sizeof(buffer) - 2);

        std::stringstream ss(str);
        std::string temp;
        std::vector<uint16_t> numbers;
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
          if (element > 4096)
          {
            error++;
            error_count++;
            element = 4095;
          }
        }
        if (error >= 1) {
          numbers.clear();
        }
        try
        {

          printf("A: %04u, B: %4u, C: %04u, D: %04u (R=%d), (E=%5d), bytes=%d\n", numbers.at(0), numbers.at(1), numbers.at(2), numbers.at(3), sample_rate, error_count, bytes_read);

          std::cout << "\033[1A"
                    << "\033[K";
          int current_values = numbers.at(0) + numbers.at(1) + numbers.at(2) + numbers.at(3);
          if (current_values != last_values)
          {
            samples++;
            last_values = current_values;
          }
        }
        catch (const std::exception &e)
        {
          sync = false;
        }
      }
    }
  }
  close(serial_fd);
}
