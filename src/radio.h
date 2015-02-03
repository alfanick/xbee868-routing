#ifndef PUT_RADIO_H
#define PUT_RADIO_H

#include "common.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <deque>

namespace PUT {
  namespace CS {
    namespace XbeeRouting {
      //! Packet ID
      typedef uint32_t PacketId;

      //! Node address
      typedef uint8_t Address;

      //! Path definition
      typedef std::deque<Address> Path;

      //! Edge definition
      typedef Address Edge[2];

      //! Cost to destination
      typedef std::pair<float, Address> Destination;

// Magic happens here
#pragma pack(push)
#pragma pack(1)

      /**
       * Frame type: 0x8A
       *
       * RF module status messages are sent from the module in response to specific conditions.
       */
      typedef struct ModemStatusFrame {
        /**
         * - 0x00 - Hardware reset
         * - 0x01 - Watchdog timer reset
         */
        uint8_t status;
      } ModemStatusFrame;

      /**
       * Frame type: 0x08 or 0x09 (for queueing command)
       *
       * Allows for module parameter registers to be queried or set.
       */
      typedef struct CommandFrame {
        //! Identifies the UART data frame for the host to correlate with a subsequent ACK (acknowledgement). If set to 0, no response is sent.
        uint8_t id;
        //! Command Name - Two ASCII characters that identify the AT Command.
        unsigned char command[2];
        //! Register data is sent as binary values. Note there are certain commands which take a null terminated array of bytes (NI/ND/DH).
        unsigned char* data;
      } CommandFrame;

      /**
       * Frame type: 0x88
       *
       * In response to an AT Command message, the module will send an AT Command Response message.
       * Some commands will send back multiple frames (for example, the ND (Node Discover) command).
       */
      typedef struct CommandResponseFrame {
        //! Identifies the UART data frame being reported. Note: If Frame ID = 0 in AT Command Mode, no AT Command Response will be given.
        uint8_t id;
        //! Command Name - Two ASCII characters that identify the AT Command.
        unsigned char command[2];
        /**
         * - 0x00 - OK
         * - 0x01 - Error
         * - 0x02 - Invalid Command
         * - 0x03 - Invalid Parameter
         */
        uint8_t status;
        //! Register data in binary format. If the register was set, then this field is not returned.
        unsigned char* data;
      } CommandResponseFrame;

      /**
       * Frame type: 0x17
       *
       * Allows for module parameter registers on a remote device to be queried or set.
       */
      typedef struct RemoteCommandFrame {
        //! Identifies the UART data frame being reported. Note: If Frame ID = 0 in AT Command Mode, no AT Command Response will be given.
        uint8_t id;
        //! Indicates the 64-bit MAC address of the remote module that is responding to the Remote AT Command request
        uint64_t mac;
        //! Set to the 16-bit network address of the remote. Set to 0xFFFE if unknown.
        uint16_t network;
        /**
         * - 0x00 - Queue Parameter Value
         * - 0x02 - Apply Value Change
         */
        uint8_t options;
        //! Name of the command. Two ASCII characters that identify the AT command
        unsigned char command[2];
        //! The register data is sent as binary values
        unsigned char* data;
      } RemoteCommandFrame;

      /**
       * Frame type: 0x97
       *
       * If a module receives a remote command response RF data frame in response
       * to a Remote AT Command Request, the module will send a Remote AT Command
       * Response message out the UART. Some commands may send back multiple frames.
       */
      typedef struct RemoteCommandResponseFrame {
        //! This is the same value passed in to the request.
        uint8_t id;
        //! The MAC address of the remote radio returning this response.
        uint64_t mac;
        //! Set to match the 16-bit network address of the destination, MSB first, LSB last. Set to 0xFFFE for broadcast TX, or if the network address is unknown
        uint16_t network;
        //! Name of the command
        unsigned char command[2];
        /**
         * - 0x00 - OK
         * - 0x01 - Error
         * - 0x02 - Invalid Command
         * - 0x03 - Invalid Parameter
         * - 0x04 - TX Failure
         */
        uint8_t status;
        //! Register data in binary format. If the register was set, then this field is not returned.
        unsigned char* data;
      } RemoteCommandResponseFrame;

      /**
       * Frame Type: 0x10
       *
       * A TX Request message will cause the module to send RF Data as an RF Packet.
       */
      typedef struct TransmitFrame {
        //! Identifies this frame, so when you receive a Transmit Status frame, you can tell that it matches this request. Setting Frame ID to ‘0' will disable the Transmit Status frame.
        uint8_t id;
        //! For broadcast use address 0x000000000000FFFF (remember LSB/MSB!)
        uint64_t mac;
        //! Set to 0xFFFE for this product.
        uint16_t network;
        //! Set to 0x00
        uint8_t radius;
        /**
         * - 0x01 - Disable ACK
         * - 0x10 - Duty purge packet. If a packet would be delayed because of duty cycle, then purge the packet.
         */
        uint8_t options;
        //! Up to 256 Bytes per packet
        unsigned char* data;
      } TransmitFrame;

      /**
       * Frame Type: 0x11
       *
       * Allows application addressing layer fields (endpoint and cluster ID) to be
       * specified for a data transmission.
       */
      typedef struct ExplicitTransmitFrame {
        //! Set to a value that will be passed back in the Tx Status frame. 0 disables the Tx Status frame.
        uint8_t id;
        //! For broadcast use address 0x000000000000FFFF (remember LSB/MSB!)
        uint64_t mac;
        //! Set to 0xFFFE for this product.
        uint16_t network;
        uint8_t source_endpoint;
        uint8_t destination_endpoint;
        uint16_t cluster;
        uint16_t profile;
        //! 0x00 for this product.
        uint8_t radius;
        /**
         * - 0x01 - Disable ACK
         * - 0x10 - Duty purge packet. If a packet would be delayed because of duty cycle, then purge the packet.
         */
        uint8_t options;
        //! Up to 72 Bytes per packet
        unsigned char* data;
      } ExplicitTransmitFrame;

      /**
       * Frame Type: 0x8B
       *
       * When a TX Request is completed, the module sends a TX Status message.
       * This message will indicate if the packet was transmitted successfully or
       * if there was a failure.
       */
      typedef struct StatusFrame {
        //! This value was used on the Transmit or Explicit transmit frame so that the status can be correlated.
        uint8_t id;
        //! Set to 0xFFFE for this product.
        uint16_t network;
        //! The number of retries it took to send the packet.
        uint8_t retries;
        /**
         * - 0x00 - Success
         * - 0x01 - MAC ACK Failure
         * - 0x03 - Purged
         */
        uint8_t status;
        //! 0x00 - No Discovery Overhead
        uint8_t discovery;
      } StatusFrame;

      /**
       * Frame Type: 0x90
       *
       * When the module receives an RF packet, it is sent out the UART using this
       * message type if AO is set to 0.
       */
      typedef struct ReceiveFrame {
        //! Source MAC address of the packet
        uint64_t mac;
        //! Always 0xFFFE for this product.
        uint16_t network;
        /**
         * - 0x01 - Packet Acknowledged
         * - 0x02 - Packet was a broadcast packet
         */
        uint8_t options;
        //! Up to 256 bytes.
        unsigned char* data;
      } ReceiveFrame;

      /**
       * Frame Type: 0x91
       *
       * When the modem receives a RF packet it is sent out the UART using this
       * message type if AO is set to 1.
       */
      typedef struct ExplicitReceiveFrame {
        //! Destination 64-bit (extended) MAC address. Set to 0xFFFF for broadcast.
        uint64_t mac;
        //! Always 0xFFFE for this product.
        uint16_t network;
        //! Endpoint of the source that initiated the transmission.
        uint8_t source_endpoint;
        //! Endpoint of the destination the message is addressed to.
        uint8_t destination_endpoint;
        //! Cluster ID the packet was addressed to.
        uint16_t cluster;
        //! Profile ID the packet was addressed to.
        uint16_t profile;
        /**
         * - 0x01 - Packet Acknowledged
         * - 0x02 - Packet was a broadcast packet
         */
        uint8_t options;
        //! Up to 256 bytes.
        unsigned char* data;
      } ExplicitReceiveFrame;


      /**
       * General Xbee frame definition.
       */
      typedef struct Frame {
        /**
         * Length of frame.
         *
         * Internally number of bytes between the length and the checksum.
         * However, for user, length is the length of the data field (!).
         */
        uint16_t length;

        /**
         * Frame type, depending on the frame type,
         * specific data union field should be filled.
         */
        enum class Type : uint8_t {
          //! Modem Status Frame - ModemStatusFrame
          ModemStatus = 0x8a,
          //! AT Command Frame - CommandFrame
          Command = 0x08,
          //! AT Command - Queue Parameter Value Frame - CommandFrame
          CommandQueue = 0x09,
          //! AT Command Response Frame - CommandResponseFrame
          CommandResponse = 0x88,
          //! Remote AT Command Request Frame - RemoteCommandFrame
          RemoteCommand = 0x17,
          //! Remote Command Response Frame - RemoteCommandResponseFrame
          RemoteCommandResponse = 0x97,
          //! Transmit Frame - TransmitFrame
          Transmit = 0x10,
          //! Explicit Addressing Command Frame - ExplicitTransmitFrame
          ExplicitTransmit = 0x11,
          //! Trasmit Status - StatusFrame
          Status = 0x8b,
          //! Receive Packet Frame - ReceiveFrame
          Receive = 0x90,
          //! Explicit Addressing Receive Frame - ExplicitReceiveFrame
          ExplicitReceive = 0x91
        } type;

        /**
         * Union translating frame contents to frame structs
         */
        union {
          //! Modem Status Frame - ModemStatusFrame
          ModemStatusFrame modem_status;
          //! AT Command Frame or Queue Parameter Value Frame - CommandFrame
          CommandFrame command;
          //! AT Command Response Frame - CommandResponseFrame
          CommandResponseFrame command_response;
          //! AT Command Response Frame - CommandResponseFrame
          RemoteCommandFrame remote_command;
          //! Remote AT Command Request Frame - RemoteCommandFrame
          RemoteCommandResponseFrame remote_command_response;
          //! Transmit Frame - TransmitFrame
          TransmitFrame transmit;
          //! Explicit Addressing Command Frame - ExplicitTransmitFrame
          ExplicitTransmitFrame explicit_transmit;
          //! Trasmit Status - StatusFrame
          StatusFrame status;
          //! Receive Packet Frame - ReceiveFrame
          ReceiveFrame receive;
          //! Explicit Addressing Receive Frame - ExplicitReceiveFrame
          ExplicitReceiveFrame explicit_receive;
        } data;

        /**
         * Checksum of the frame, calculated automatically
         */
        uint8_t checksum;

        /**
         * True size of unserialized frame
         */
        int size;

        /**
         * Create frame of given type
         *
         * @param t Type of frame
         */
        Frame(Type t) : length(0), type(t) { };

        /**
         * Destroys frame, frees memory used by data (if any).
         */
        ~Frame() {
          if (length == 0)
            return;

          switch (type) {
            case Type::Command:
            case Type::CommandQueue:
              free(data.command.data);
              break;

            case Type::CommandResponse:
              free(data.command_response.data);
              break;

            case Type::RemoteCommand:
              free(data.remote_command.data);
              break;

            case Type::RemoteCommandResponse:
              free(data.remote_command_response.data);
              break;

            case Type::Transmit:
              free(data.transmit.data);
              break;

            case Type::ExplicitTransmit:
              free(data.explicit_transmit.data);
              break;

            case Type::Receive:
              free(data.receive.data);
              break;

            case Type::ExplicitReceive:
              free(data.explicit_receive.data);

            default:
              break;
          }
        }

#define INSERT_STRUCT(_f, _l) memcpy(&frame[3+length], &data._f, _l); length += _l;
#define INSERT_DATA(_f) memcpy(&frame[3+length], data._f.data, offset); length += offset;

        /**
         * Serialize current frame into byte array used for communication with
         * Xbee modem.
         *
         * @param packet_length Length of generated packet
         * @return Byte array ready to send to Xbee
         */
        unsigned char* serialize(int &packet_length) {
          unsigned char* frame = (unsigned char*)malloc(300);
          uint16_t offset = length;
          length = 0;

          frame[0] = 0x7e;
          frame[3 + length++] = (unsigned char)type;

          switch (type) {
            case Type::ModemStatus:
              INSERT_STRUCT(modem_status, 1);
              break;

            case Type::Command:
            case Type::CommandQueue:
              INSERT_STRUCT(command, 3);
              INSERT_DATA(command);
              break;

            case Type::CommandResponse:
              INSERT_STRUCT(command_response, 4);
              INSERT_DATA(command_response);
              break;

            case Type::RemoteCommand:
              INSERT_STRUCT(remote_command, 14);
              INSERT_DATA(remote_command);
              break;

            case Type::RemoteCommandResponse:
              INSERT_STRUCT(remote_command_response, 14);
              INSERT_DATA(remote_command_response);
              break;

            case Type::Transmit:
              INSERT_STRUCT(transmit, 13);
              INSERT_DATA(transmit);
              break;

            case Type::ExplicitTransmit:
              INSERT_STRUCT(explicit_transmit, 19);
              INSERT_DATA(explicit_transmit);
              break;

            case Type::Status:
              INSERT_STRUCT(status, 6);
              break;

            case Type::Receive:
              INSERT_STRUCT(receive, 11);
              INSERT_DATA(receive);
              break;

            case Type::ExplicitReceive:
              INSERT_STRUCT(explicit_receive, 17);
              INSERT_DATA(explicit_receive);
              break;
          }

          frame[1] = (length >> 8);
          frame[2] = (length);

          checksum = 0;

          for (int i = 0; i < length; i++)
            checksum += frame[3 + i];

          checksum = (uint8_t)0xff - checksum;
          frame[3 + length] = checksum;

          packet_length = 4 + length;

          length = offset;

          return frame;
        }

#define READ_STRUCT(_f,_l) memcpy(&data._f, &frame[4], _l); offset += _l;
#define READ_DATA(_f) data._f.data = (unsigned char*)malloc(length-offset+1); memcpy(data._f.data, &frame[4+offset], length - offset-1);

        /**
         * Unserializes byte array received from Xbee radio into
         * current frame.
         *
         * @param frame Byte array representing frame received from Xbee
         */
        void unserialize(unsigned char* frame) {
          uint16_t offset = 0;

          length = frame[2] | (((uint16_t)frame[1]) << 8);
          type = (Type)frame[3];
          checksum = frame[3 + length];

          switch (type) {
            case Type::ModemStatus:
              READ_STRUCT(modem_status, 1);
              break;

            case Type::Command:
            case Type::CommandQueue:
              READ_STRUCT(command, 3);
              READ_DATA(command);
              break;

            case Type::CommandResponse:
              READ_STRUCT(command_response, 4);
              READ_DATA(command_response);
              break;

            case Type::RemoteCommand:
              READ_STRUCT(remote_command, 14);
              READ_DATA(remote_command);
              break;

            case Type::RemoteCommandResponse:
              READ_STRUCT(remote_command_response, 14);
              READ_DATA(remote_command_response);
              break;

            case Type::Transmit:
              READ_STRUCT(transmit, 13);
              READ_DATA(transmit);
              break;

            case Type::ExplicitTransmit:
              READ_STRUCT(explicit_transmit, 19);
              READ_DATA(explicit_transmit);
              break;

            case Type::Status:
              READ_STRUCT(status, 6);
              break;

            case Type::Receive:
              READ_STRUCT(receive, 11);
              READ_DATA(receive);
              break;

            case Type::ExplicitReceive:
              READ_STRUCT(explicit_receive, 17);
              READ_DATA(explicit_receive);
              break;
          }

          size = offset;
          length -= offset + 1;
        }

        //! Broadcast address
        static const unsigned long long int BROADCAST = 0xFFFF000000000000;
      } Frame;

// End of magic
#pragma pack(pop)


    }
  }
};

#endif
