#ifndef PUT_RADIO_XBEE_H
#define PUT_RADIO_XBEE_H

#include <string>
#include <mutex>

#include "node.h"
#include "../radio.h"

namespace PUT {
  namespace CS {
    namespace XbeeRouting {
      /**
       * Communication with Xbee radio using UART. Thread safe.
       *
       * Communication is done using Frame, Packet may be converted to Frame
       * if needed.
       *
       * @see Packet::to_frame()
       * @see Packet::from_frame()
       */
      class Xbee {
       private:
        //! Serial port descriptor
        int serial;
        //! Serial port lock
        std::mutex serial_mutex;
        //! Serial port device path
        std::string device;
        //! Network ID
        uint16_t network;

       public:
        /**
         * Create instance of Xbee connection and gets essential paremeters.
         *
         * After opening serial port, Xbee radio is restarted (FR command).
         * Then parameters like serial number (address), node identifier (network),
         * network address and power supply voltage are requested. They may be processed
         * in Router.
         *
         * @param d Serial port device path
         */
        Xbee(std::string d);

        /**
         * Destroy connection and close serial port
         */
        ~Xbee();

        /**
         * Blocking. Send simple AT command (usually parameter request)
         * and wait for anwser. Every other frame is dropped!
         *
         * @param cmd 2-byte AT command
         * @param length Length of returned value
         * @return Value
         */
        char* get(const char* cmd, uint8_t &length);

        /**
         * Send simple AT command to local device.
         *
         * @param cmd 2-byte AT command
         */
        void command(const char* cmd);

        /**
         * Send simple AT command to remote (but adjacent) device).
         *
         * @param node Node descriptor
         * @param cmd 2-byte AT command
         */
        void command(Node* node, const char* cmd);

        /**
         * Send AT command with parameters to local device.
         *
         * @param cmd 2-byte AT command
         * @param params AT command parameters (according to command description)
         * @param length Number of bytes in command parameter
         */
        void command(const char* cmd, unsigned char* params, uint8_t length);

        /**
         * Send AT command with parameters to remote device.
         *
         * @param node Node descriptor
         * @param cmd 2-byte AT command
         * @param params AT command parameters (according to command description)
         * @param length Number of bytes in command parameter
         */
        void command(Node* node, const char* cmd, unsigned char* params, uint8_t length);

        /**
         * Blocking. Wait and return for frame from Xbee radio.
         *
         * Does some processing on a frame, but never hold or purge a frame.
         *
         * If frame is Frame::Type::Receive, ATDB command is sent to get RSSI.
         * If frame is Frame::Type::Status, quality_error and quality_retry are updated.
         * Commands responses for DB, %V and NI update last RSSI, power supply voltage
         * and network address.
         *
         * @return Received parsed frame.
         */
        Frame* receive();

        /**
         * Send frame to radio.
         *
         * @param frame Frame to send.
         */
        void send(Frame* frame);

        /**
         * Send text to adjacent node.
         *
         * @param node Node descriptor
         * @param data Text
         */
        void send(Node* node, std::string data);

        /**
         * Broadcast text (using broadcast frame, not map).
         *
         * @param data Text
         */
        void broadcast(std::string data);
      };
    }
  }
}
#endif
