#ifndef PUT_RADIO_DISPATCHER_H
#define PUT_RADIO_DISPATCHER_H

#include "../radio.h"

#include "network.h"
#include "xbee.h"
#include "packet.h"
#include "history.h"
#include "../driver/driver.h"

#include <chrono>
#include <unordered_map>
#include <bitset>
#include <mutex>
#include <thread>
#include <atomic>
#include <deque>

namespace PUT {
  namespace CS {
    namespace XbeeRouting {
      typedef int32_t Timeout;

      /**
       * Dispatcher maintains delivery status of packet.
       *
       * There are three types of packet delivery:
       *   - Dispatcher::send() - send Packet to next node, doesn't care about status
       *   - Dispatcher::broadcast() - broadcast Packet to adjacent nodes
       *   - Dispatcher::deliver() - ensure Packet is delivered, repeat if not delivered, send ACK, watch for status
       *
       * Dispatcher scans every incoming packet and updates history or send ACK if status changed.
       * If Packet::type is Packet::Type::Ack and Packet::status > 0, then Packet::status indicates it was not possible
       * to deliver the Packet to Node with address Packet::status, otherwise (if eq 0) packet was delivered to destination.
       *
       * @todo Design and throw exceptions
       */
      class Dispatcher {
       private:
        //! Network graph
        Network &network;
        //! Xbee connection
        Xbee &xbee;
        //! Self node
        Node* self;
        //! History
        History history;

        Driver &driver;

        /**
         * Thread responsible for executing tick method.
         *
         * Once in a while tick() checks if there are outdated packets that need to be retransmitted
         */
        std::thread tick_thread;

        /**
         * Tick_thread status.
         *
         * "Ticking" is enabled when true.
         */
        std::atomic_bool tick_threadRun;


        /**
         * Sleep time for tick_thread
         */
        const std::chrono::milliseconds tick_sleep_time = std::chrono::milliseconds(100); //! TODO: set proper delay between invocation of tic()

        /**
         * processing time on single node, timeout math
         * see [Inz] Timeouts math google doc
         */
        const uint8_t tv = 1;

        /**
         * constant in timeouth math
         * see [Inz] Timeouts math google doc
         */
        const uint8_t c = 2;

        /**
         * non-source-node timeout multiplier
         *
         * When ack doesn't come back through current node, but packet was going through this node, memory in history
         * have to be released. The Idea is to set timeout for this packet, but multiplied by this constant to avoid receiving
         * ack for packet which is removed from history.
         */
        const uint8_t timeout_multiplier = 100;     // TODO: set proper value!!!

        /**
         * antireliability treshold, if antireliability is bigger and dispatcher failed to retransmit packet, PACKET::Type::EdgeDrop is broadcasted
         */
        const float antireliability_trheshold = 10;  // TODO: set proper value!!!

       public:
        /**
         * Create new Dispatcher instance. Does nothing.
         *
         * @param x Xbee Radio
         * @param n Network graph (must contain self already)
         */
        Dispatcher(Xbee &x, Network &n, Driver &d);

        /**
         * Destroy dispatcher.
         *
         * Every packet still in history must be released.
         * Probably ACK with error on self node should be send.
         */
        ~Dispatcher();

        /**
         * Reliably send Packet to its destination.
         *
         * Packet is given next free ID and it is stored in Dispatcher::history.
         * Packet is send to next adjacent node according to Network::path().
         * If packet is delivered successful ACK is awaited, otherwise Packet delivery
         * is repeated with new path.
         *
         *
         * @param p Complete Packet (must contain source, destination and type).
         * @return True if any path to destination exists
         * @see Dispatcher::watch()
         * @see Dispatcher::scan()
         */
        bool deliver(Packet* p);

        /**
         * Retransmit packet to its destination
         *
         * Similar to deliver, but instead of invoking watch(), that create metadata for packet and
         * adds new entry to history, this method just change metadata in existing entry in history.
         *
         * @param meta Pointer to metadata in history
         * @return True if any path to destination exists
         * @see Dispatcher::deliver()
         * @see Dispatcher::scan()
         */
        bool retransmit(Metadata* meta);

        /**
         * Tries to retransmit packet to its destination
         *
         * Checks if packet's retransmission_counter is lower than retransmission_max constant adn invoke
         * retransmit() method.
         *
         * @param meta Pointer to metadata in history
         * @return True if any path to destination exists
         * @see Dispatcher::deliver()
         * @see Dispatcher::scan()
         */
        bool try_retransmit(Metadata* meta);

        /**
         * Send packet to its adjacent destination.
         *
         * This method doesn't care if packet delivery was successful. Send assumes
         * that Packet::destination is adjacent, however if MAC address is known
         * and destination is not adjacent, packet will still be send.
         *
         * @param p Complete Packet
         * @return True if any mac address is known
         */
        bool send(Packet* p);

        /**
         * Broadcast packet to adjacent nodes.
         *
         * Strange things may happen if packet is still routed when
         * received by adjacent node. DO NOT USE!
         *
         * @param p Packet (destination is changed to 0, to mark broadcast packet)
         */
        void broadcast(Packet* p);

        /**
         * Tick maintains delivery of packets with timeouted ACK.
         *
         * Checks packet history, if some packet awaits for ACK longer then
         * its timeout, packet should delivered once again if possible
         * (delivery number should be limited), otherwise ACK with error on
         * next node should be send.
         *
         * @return Number of timeouted packets.
         * @todo(rybmat) Timeout system must be designed - nothing in this matter is done yet
         */
        int tick();

        /**
         * Scan every incoming packet for delivery messages.
         *
         * This method ensures proper ACK messaging.
         *
         * If incoming packet is Packet::Type::Data and its destination is self,
         * then it means the packet reached its destination - new ACK should be
         * send to last hop.
         *
         * If Packet::Type::Internal and Frame::Type::Status, scan() updates
         * packet history with status data and updates the network. A delay
         * is calculated here. The frame is removed from history_frames and local
         * frame_id is released. Even is packet was undelivered, frame_id no
         * longer needs to be the same - it will be changed with next delivery.
         * If packet was undelivered it should be repeated few times.
         *
         * If Packet::Type::Ack, it means some packet status has changed.
         * If ACK is successful, local RemoteParameters from packet history
         * should be inserted and ACK should be send to origin of the packet,
         * the packet is removed from packet history.
         * If ACK is not successful, packet should repeated few times, otherwise
         * not successful ACK should be send to origin.
         *
         * @param p Incoming packet
         * @see Dispatcher::watch()
         * @see Dispatcher::deliver()
         */
        void scan(Packet* p);

        inline void handle_data(Packet* packet);
        inline void handle_ack(Packet* packet);
        inline void handle_internal(Packet* packet);

       public:
        /**
         * calculates timeout value for packet
         */
        std::chrono::steady_clock::time_point timeout(Packet* packet, Path &path);


        /**
         * Sends ack with given status
         */
        void send_ack(Packet* packet, Address status);


        /**
         * broadcasts edge drop for given edge a->b
         */
        void broadcast_edge_drop(Address a, Address b);
      };
    }
  }
}
#endif
