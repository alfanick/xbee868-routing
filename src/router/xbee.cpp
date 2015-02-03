#include "xbee.h"

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <math.h>
#include <stdint.h>

namespace PUT {
  namespace CS {
    namespace XbeeRouting {
      Xbee::Xbee(std::string d) : device(d) {
        serial_mutex.lock();

        serial = open(device.c_str(), O_RDWR | O_NOCTTY);

        struct termios options;

        tcgetattr(serial, &options);

        cfsetispeed(&options, B115200);
        cfsetospeed(&options, B115200);
        options.c_cflag |= CLOCAL | CREAD;
        options.c_cflag &= ~PARENB;
        options.c_cflag |= CSTOPB;
        options.c_cflag &= ~CSIZE;
        options.c_cflag |= CS8;
        options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
        options.c_iflag &= ~(IXON | IXOFF | IXANY);
        options.c_oflag &= ~OPOST;

        tcsetattr(serial, TCSANOW, &options);

        fcntl(serial, F_SETFL, 0);

        if (getenv("IN_SIMULATOR") == NULL)
          sleep(1);

        tcflush(serial, TCIOFLUSH);

        serial_mutex.unlock();

        // reset
        command("FR");

        if (getenv("IN_SIMULATOR") == NULL)
          sleep(1);
      }

      Xbee::~Xbee() {
        serial_mutex.lock();
        close(serial);
      }

      char* Xbee::get(const char* cmd, uint8_t &length) {
        command(cmd);

        Frame* frame;
        bool should_run = true;
        char* response;
        length = 0;

        while (should_run) {
          frame = receive();

          if (strcmp(cmd, (char*)frame->data.command_response.command) == 0) {
            should_run = false;
            length = frame->length;
            response = (char*)malloc(length * sizeof(char));
            memcpy(response, (char*)frame->data.command_response.data, length);
          }

          delete frame;
        }

        return response;
      }

      void Xbee::command(const char* cmd) {
        command(cmd, NULL, 0);
      }

      void Xbee::command(Node* node, const char* cmd) {
        command(node, cmd, NULL, 0);
      }

      void Xbee::command(const char* cmd, unsigned char* params, uint8_t length) {
        Frame* request = new Frame(Frame::Type::Command);

        request->data.command.id = 1;
        memcpy(&request->data.command.command, cmd, 2);

        if (length > 0) {
          request->length = length;
          request->data.command.data = (unsigned char*)malloc(length * sizeof(unsigned char));
          memcpy(request->data.command.data, params, length);
        }

        send(request);

        delete request;
      }

      void Xbee::command(Node* node, const char* cmd, unsigned char* params, uint8_t length) {
        Frame* request = new Frame(Frame::Type::RemoteCommand);

        request->data.remote_command.id = 1;
        memcpy(&request->data.remote_command.command, cmd, 2);

        request->data.remote_command.mac = node->mac;
        request->data.remote_command.network = node->network;
        request->data.remote_command.options = 0x00;

        if (length > 0) {
          request->length = length;
          memcpy(request->data.remote_command.data, params, length);
        }

        send(request);

        delete request;
      }

      Frame* Xbee::receive() {
        unsigned char* packet = (unsigned char*)malloc(300);
        int offset = 0, length = 0;
        int status = 0;
        unsigned char c;

        Frame* frame = new Frame((Frame::Type)0x00);

        while ((status = read(serial, &c, 1))) {
          if (status == -1) {
            fprintf(stderr, "Hardware disconnected\n");
            fflush(stderr);
            exit(1);
          }

          if (c == 0x7E && offset == 0) {
          } else if (offset == 1) {
            length = ((uint16_t)c) << 8;
          } else if (offset == 2) {
            length |= ((uint16_t)c);
          }

          packet[offset++] = c;

          if (offset == length + 4) {
#ifndef RASPBERRY
            printf("\033[0;33m");

            for (int i = 0; i < offset; i++) {
              printf("%.2X ", packet[i]);
            }

            printf("\033[0m\n");
#endif

            frame->unserialize(packet);

            break;
          }
        }

        free(packet);

        return frame;
      }

      void Xbee::send(Frame* frame) {
        int length, status;
        unsigned char* packet = frame->serialize(length);

        serial_mutex.lock();
        status = write(serial, packet, length);
        serial_mutex.unlock();

        if (status == -1) {
          fprintf(stderr, "Hardware disconnected\n");
          fflush(stderr);
          exit(1);
        }

#ifndef RASPBERRY
        printf("\033[0;32m");

        for (int i = 0; i < length; i++) {
          printf("%.2X ", packet[i]);
        }

        printf("\033[0m\n");
#endif

        free(packet);
      }

      void Xbee::send(Node* node, std::string data) {
        Frame* frame = new Frame(Frame::Type::Transmit);

        frame->data.transmit.id = (rand() % 0xFE) + 1;
        frame->data.transmit.mac = node->mac;
        frame->data.transmit.network = node->network;
        frame->data.transmit.radius = 0x00;
        frame->data.transmit.options = 0x00;
        frame->length = data.size();
        frame->data.transmit.data = (unsigned char*)malloc(frame->length);
        memcpy(frame->data.transmit.data, data.c_str(), frame->length);

        send(frame);

        delete frame;
      }

      void Xbee::broadcast(std::string data) {
        Frame* frame = new Frame(Frame::Type::Transmit);

        frame->data.transmit.id = 0x00;
        frame->data.transmit.mac = Frame::BROADCAST;
        frame->data.transmit.network = network;
        frame->data.transmit.radius = 0x00;
        frame->data.transmit.options = 0x00;
        frame->length = data.size();
        frame->data.transmit.data = (unsigned char*)malloc(frame->length);
        memcpy(frame->data.transmit.data, data.c_str(), frame->length);

        send(frame);

        delete frame;
      }
    }
  }
}
