#include <iostream>

extern "C" {
#include "uP7.h"
}

#include "uP7proxyApi.h"

#include <cstdlib>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

void print_error(const char* context)
{
    DWORD error_code = GetLastError();
    char buffer[256];
    DWORD size = FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_MAX_WIDTH_MASK,
        NULL, error_code, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
        buffer, sizeof(buffer), NULL);
    if (size == 0) {
        buffer[0] = 0;
    }
    fprintf(stderr, "%s: %s\n", context, buffer);
}

void die(const char* s)
{
    print_error(s);
    exit(1);
}

HANDLE open_serial(const char* device, uint32_t baudrate)
{
    HANDLE port = CreateFileA(device, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (port == INVALID_HANDLE_VALUE) {
        print_error(device);
        return INVALID_HANDLE_VALUE;
    }

    // Flush away any bytes previously read or written.
    BOOL success = FlushFileBuffers(port);
    if (!success) {
        print_error("Failed to flush serial port");
        CloseHandle(port);
        return INVALID_HANDLE_VALUE;
    }

    // Configure read and write operations to timeout with 50ms
    COMMTIMEOUTS timeouts = { 0 };
    timeouts.ReadIntervalTimeout = 0;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 0;

    success = SetCommTimeouts(port, &timeouts);
    if (!success) {
        print_error("Failed to set serial timeouts");
        CloseHandle(port);
        return INVALID_HANDLE_VALUE;
    }

    // Set the baud rate and other options.
    DCB state = { 0 };
    state.DCBlength = sizeof(DCB);
    state.BaudRate = baudrate;
    state.ByteSize = 8;
    state.Parity = NOPARITY;
    state.StopBits = ONESTOPBIT;
    success = SetCommState(port, &state);
    if (!success) {
        print_error("Failed to set serial settings");
        CloseHandle(port);
        return INVALID_HANDLE_VALUE;
    }

    return port;
}

int main(int arg_num, char* args[])
{
    const uint32_t STRMAXLEN = 1024;
    // check args
    if (arg_num < 7) {
        printf("Usage:\n  up7_serial_proxy [preprocessor_artifacts_path] [mcu_port] [mcu_baudrate] [mcu_timer_frequency_hz] [baical_ip] [baical_port]\n");
        printf("Example:\n  ./up7_serial_proxy.exe ..\\\\..\\\\..\\\\up7_client\\\\up7\\\\preproc COM3 115200 1000 127.0.0.1 9009\n");
        return -1;
    }
    // parse args
    wchar_t preproc_path[STRMAXLEN];
    mbstowcs_s(NULL, preproc_path, STRMAXLEN, args[1], strlen(args[1]) + 1);
    char* mcu_port = args[2];
    uint32_t mcu_baudrate = (uint32_t)strtoul(args[3], nullptr, 0);
    uint16_t mcu_timer_freq = (uint16_t)strtoul(args[4], nullptr, 0);
    wchar_t baical_ip[STRMAXLEN];
    mbstowcs_s(NULL, baical_ip, STRMAXLEN, args[5], strlen(args[5]) + 1);
    uint16_t baical_port = (uint16_t)strtoul(args[6], nullptr, 0);

    // print args
    printf("Parse arguments:\n");
    printf("  Preprocessor artifacts path: %S\n", preproc_path);
    printf("  MCU COM port:                %s\n", mcu_port);
    printf("  MCU baudrate:                %d\n", mcu_baudrate);
    printf("  MCU timer frequency [Hz]:    %d\n", mcu_timer_freq);
    printf("  Baical UDP IP:               %S\n", baical_ip);
    printf("  Baical UDP port:             %d\n", baical_port);

    // open serial port
    printf("Open serial port...\n");
    HANDLE mcu_serial = open_serial(mcu_port, mcu_baudrate);
    if (mcu_serial == INVALID_HANDLE_VALUE) {
        die("Failed to open COM port");
    }

    // init proxy
    printf("Init proxy...\n");
    wchar_t proxy_cfg[STRMAXLEN];
    swprintf_s(proxy_cfg, STRMAXLEN, TM("/P7.Name=up7_serial_proxy /P7.Verb=0 /P7.Addr=%s /P7.Port=%d /P7.Sink=Baical /P7.Pool=4096"), baical_ip, baical_port);
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
    DWORD len;
    char buffer[BUFLEN];
    bool read_success;
    while (1) {
        read_success = ReadFile(mcu_serial, buffer, BUFLEN, &len, NULL);
        if (!read_success) {
            die("Failed to read from port\n");
        } else if (len) {
            //printf("  Read %d bytes from port\n", len);
            if (!g_pFifo->Write(buffer, len)) {
                die("Failed to write to Baikal");
            }
        }
    }
}
