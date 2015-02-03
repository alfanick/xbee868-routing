#ifndef PUT_RADIO_NODE_H
#define PUT_RADIO_NODE_H

#include <string>
#include <stdint.h>
#include <map>
#include <memory>
#include <chrono>

#include "packet.h"

namespace PUT {
  namespace CS {
    namespace XbeeRouting {
      /**
       * Represents single node in network.
       *
       * Node is identified using its logical Address and Xbee serial number
       * (called MAC).
       */
      class Node {
       public:
        //! 8-byte Xbee radio MAC address
        uint64_t mac;

        //! Xbee radio name (ATNI command)
        std::string name;

        //! Address
        Address address;

        //! Network ID (based on self)
        uint16_t network;

        //! True if node is self
        bool self;

        std::chrono::steady_clock::time_point last_tick;

        /**
         * Create empty node
         */
        Node() : mac(0), name("[EMPTY]"), address(0), network(0), self(false) {
          last_tick = std::chrono::steady_clock::now();
        };


        /**
         * Create node with given parameters.
         *
         * @param m MAC number (reversed bytes order)
         * @param nt Network identifier
         * @param n Name
         * @param a Logical address
         */
        Node(uint64_t m,  uint16_t nt, std::string n, Address a) : mac(m), name(n), address(a), network(nt), self(false) {
          last_tick = std::chrono::steady_clock::now();
        };

        //! Destroy node
        ~Node();
      };
    }
  }
}
#endif
