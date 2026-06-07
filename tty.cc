#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

int configure_serial_port(int fd, speed_t baud_rate) {
  struct termios tty;

  // Read existing settings to use as a baseline
  if (tcgetattr(fd, &tty) != 0) {
    fprintf(stderr, "Error %d from tcgetattr: %s\n", errno, strerror(errno));
    return -1;
  }

  // --- Control Modes (c_cflag) ---
  tty.c_cflag &= ~PARENB;  // Clear parity bit -> No parity (N)
  tty.c_cflag &= ~CSTOPB;  // Clear stop field -> 1 stop bit (1)
  tty.c_cflag &= ~CSIZE;   // Clear the current character size mask
  tty.c_cflag |= CS8;      // Set 8 data bits (8)

  // Enable Hardware Flow Control (RTS/CTS)
  tty.c_cflag |= CRTSCTS;

  tty.c_cflag |=
      CREAD | CLOCAL;  // Turn on READ & ignore modem control lines (local line)

  // --- Local Modes (c_lflag) ---
  // Disable canonical mode (line-by-line processing), echoing, and signaling
  // chars. This forces "raw" mode for immediate, byte-by-byte full-duplex I/O.
  tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHONL | ISIG);

  // --- Input Modes (c_iflag) ---
  // Disable software flow control (XON/XOFF) and disable translation of
  // carriage returns/newlines
  tty.c_iflag &= ~(IXON | IXOFF | IXANY);
  tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);

  // --- Output Modes (c_oflag) ---
  // Prevent special interpretation or remapping of output bytes (like \n to
  // \r\n)
  tty.c_oflag &= ~OPOST;
  tty.c_oflag &= ~ONLCR;

  // --- VMIN and VTIME ---
  // VMIN = 0, VTIME = 0 means non-blocking read: read() returns immediately
  // with whatever bytes are available in the system buffer.
  tty.c_cc[VMIN] = 0;
  tty.c_cc[VTIME] = 0;

  // --- Set Baud Rate ---
  cfsetispeed(&tty, baud_rate);
  cfsetospeed(&tty, baud_rate);

  // Clean the line and apply the settings immediately
  tcflush(fd, TCIFLUSH);
  if (tcsetattr(fd, TCSANOW, &tty) != 0) {
    fprintf(stderr, "Error %d from tcsetattr: %s\n", errno, strerror(errno));
    return -1;
  }

  return 0;
}

int main(int argc, char *argv[]) {
  // Default port, but you can pass one via CLI args
  const char *port_name = (argc > 1) ? argv[1] : "/dev/ttyUSB0";

  printf("Opening serial port: %s\n", port_name);

  // Open the device in Read/Write mode.
  // O_NOCTTY: Prevents the port from becoming the process's controlling
  // terminal. O_NONBLOCK: Ensures open() doesn't hang waiting for carrier
  // detect (DCD) lines.
  int fd = open(port_name, O_RDWR | O_NOCTTY | O_NONBLOCK);

  if (fd < 0) {
    fprintf(stderr, "Error %d opening %s: %s\n", errno, port_name,
            strerror(errno));
    return EXIT_FAILURE;
  }

  // Configure for 8N1 at 1200 Baud (change B115200 to B921600, etc., as
  // needed)
  if (configure_serial_port(fd, B1200) < 0) {
    close(fd);
    return EXIT_FAILURE;
  }

  printf(
      "Port successfully configured to 8N1, 1200 bps, Hardware Flow "
      "Control.\n");
  printf("Starting full-duplex loop. Press Ctrl+C to exit.\n\n");

  char read_buf[256];
  const char *tx_msg = "Ping\r\n";

  // Simple asynchronous/full-duplex demonstration loop
  while (1) {
    // 1. Write data out (Full-duplex output)
    // In a real application, you'd likely pace this or use select()/poll()
    write(fd, tx_msg, strlen(tx_msg));

    // 2. Read data in (Full-duplex input)
    memset(read_buf, 0, sizeof(read_buf));
    int num_bytes = read(fd, read_buf, sizeof(read_buf) - 1);

    if (num_bytes > 0) {
      printf("Received %d bytes: ", num_bytes);
      // Print out safely as characters or hex values
      for (int i = 0; i < num_bytes; i++) {
        if (read_buf[i] >= 32 && read_buf[i] <= 126) {
          putchar(read_buf[i]);
        } else {
          printf("[%02X]", (unsigned char)read_buf[i]);
        }
      }
      printf("\n");
    } else if (num_bytes < 0 && errno != EAGAIN) {
      fprintf(stderr, "Error reading: %s\n", strerror(errno));
      break;
    }

    // Small sleep to prevent eating 100% CPU in this basic loop
    usleep(100000);
  }

  close(fd);
  return EXIT_SUCCESS;
}
