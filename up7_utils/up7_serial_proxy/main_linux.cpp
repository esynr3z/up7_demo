#include <iostream>

extern "C" {
#include "uP7.h"
}

#include "uP7proxyApi.h"

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

void die(const char* s)
{
    perror(s);
    exit(1);
}

int open_serial(const char* device, uint32_t baudrate)
{
    int fd = open(device, O_RDWR | O_NOCTTY);
    if (fd == -1) {
        perror(device);
        return -1;
    }

    // Flush away any bytes previously read or written.
    int result = tcflush(fd, TCIOFLUSH);
    if (result) {
        perror("tcflush failed"); // just a warning, not a fatal error
    }

    // Get the current configuration of the serial port.
    struct termios options;
    result = tcgetattr(fd, &options);
    if (result) {
        perror("tcgetattr failed");
        close(fd);
        return -1;
    }

    // Turn off any options that might interfere with our ability to send and
    // receive raw binary bytes.
    options.c_iflag &= ~(INLCR | IGNCR | ICRNL | IXON | IXOFF);
    options.c_oflag &= ~(ONLCR | OCRNL);
    options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);

    // Set up timeouts: Calls to read() will return as soon as there is
    // at least one byte available or when 100 ms has passed.
    options.c_cc[VTIME] = 1;
    options.c_cc[VMIN] = 0;

    // This code only supports certain standard baud rates. Supporting
    // non-standard baud rates should be possible but takes more work.
    switch (baudrate) {
    case 4800:
        cfsetospeed(&options, B4800);
        break;
    case 9600:
        cfsetospeed(&options, B9600);
        break;
    case 19200:
        cfsetospeed(&options, B19200);
        break;
    case 38400:
        cfsetospeed(&options, B38400);
        break;
    case 115200:
        cfsetospeed(&options, B115200);
        break;
    default:
        fprintf(stderr, "warning: baud rate %u is not supported, using 9600.\n",
                baudrate);
        cfsetospeed(&options, B9600);
        break;
    }
    cfsetispeed(&options, cfgetospeed(&options));

    result = tcsetattr(fd, TCSANOW, &options);
    if (result) {
        perror("tcsetattr failed");
        close(fd);
        return -1;
    }

    return fd;
}

int main(int arg_num, char* args[])
{
    const uint32_t STRMAXLEN = 1024;
    // check args
    if (arg_num < 7) {
        printf("Usage:\n  up7_serial_proxy [preprocessor_artifacts_path] [mcu_port] [mcu_baudrate] [mcu_timer_frequency_hz] [baical_ip] [baical_port]\n");
        printf("Example:\n  ./up7_serial_proxy ../../../up7_client/up7/preproc /dev/ttyACM0 115200 1000 127.0.0.1 9009\n");
        return -1;
    }
    // parse args
    char* preproc_path = args[1];
    char* mcu_port = args[2];
    uint32_t mcu_baudrate = (uint32_t)strtoul(args[3], nullptr, 0);
    uint16_t mcu_timer_freq = (uint16_t)strtoul(args[4], nullptr, 0);
    char* baical_ip = args[5];
    uint16_t baical_port = (uint16_t)strtoul(args[6], nullptr, 0);

    // print args
    printf("Parse arguments:\n");
    printf("  Preprocessor artifacts path: %s\n", preproc_path);
    printf("  MCU COM port:                %s\n", mcu_port);
    printf("  MCU baudrate:                %d\n", mcu_baudrate);
    printf("  MCU timer frequency [Hz]:    %d\n", mcu_timer_freq);
    printf("  Baical UDP IP:               %s\n", baical_ip);
    printf("  Baical UDP port:             %d\n", baical_port);

    // open serial port
    printf("Open serial port...\n");
    int mcu_serial = open_serial(mcu_port, mcu_baudrate);
    if (mcu_serial < 0) {
        die("Failed to open COM port");
    }

    // init proxy
    printf("Init proxy...\n");
    char proxy_cfg[STRMAXLEN];
    sprintf(proxy_cfg, TM("/P7.Name=up7_serial_proxy /P7.Verb=0 /P7.Addr=%s /P7.Port=%d /P7.Sink=Baical /P7.Pool=4096"), baical_ip, baical_port);
    IuP7proxy* l_iProxy = uP7createProxy(proxy_cfg, preproc_path);
    if (!l_iProxy) {
        die("open proxy nullptr");
    }
    IuP7Fifo* g_pFifo = NULL;
    if (!l_iProxy->RegisterCpu(1, false, mcu_timer_freq, TM("up7_serial_proxy"), 0xFFFF, false, g_pFifo)) {
        die("register cpu fail");
    }
    printf("Listening to client...\n");
    const uint32_t BUFLEN = 2048;
    int len;
    char buffer[BUFLEN];
    while (1) {
        len = read(mcu_serial, buffer, BUFLEN);
        if (len < 0) {
            die("Failed to read from port\n");
        } else if (len) {
            //printf("  Read %d bytes from port\n", len);
            if (!g_pFifo->Write(buffer, len)) {
                die("Failed to write to Baikal");
            }
        }
    }
}
