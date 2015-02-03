#ifndef PUT_RADIO_HISTORY_H
#define PUT_RADIO_HISTORY_H

#include "../radio.h"

#include "network.h"
#include "xbee.h"
#include "packet.h"

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
      /**
       * Descriptor of Packet history.
       *
       * For every packet marked for delivery, its Metadata
       * lives in Dispatcher::history as long as its positive
       * ACK is not received.
       */
      struct Metadata {
        /**
         * Packet to watch.
         *
         * Memory must be released with removing Metadata (it means that destructor doesn't delete this)
         */
        Packet* packet;

        /**
         * Unique packet id - it is the same on every node
         */
        uint32_t packet_id; // xxFRTOID

        /**
         * Path history on current node.
         *
         * There must be at least one Path. There are more
         * if packet was repeated.
         */
        std::vector<Path> path_history;

        /**
         * Local Xbee frame ID
         */
        uint8_t frame_id;

        /**
         * Values from Xbee StatusFrame - summed if there are
         * more StatusFrames (if Packet was send multiple times)
         */
        RemoteParameters frame_status;

        /**
         * Send time of *first* frame with packet
         */
        std::chrono::steady_clock::time_point send_time;

        /**
         * Time after which packet will be retransmitted
         */
        std::chrono::steady_clock::time_point timeout;

        bool check_timeout = false;

        /**
         * Retransmission counter (by software)
         */
        std::atomic_ushort retransmission_counter {0};

        /**
         * Retransmission limiter
         * TODO set proper value
         */
        static const uint8_t retransmission_max = 5;


        // Pointer to packet is not deleted by destructor!!!
        ~Metadata();
      };

      class History {
       private:
        using PacketsHistory = std::unordered_map<PacketId, Metadata*>;
        using FramesHistory = std::unordered_map<uint8_t, Metadata*>;
        /**
         * Keeps history of send Packets until they ACK or timeout.
         *
         * Map contains Metadata for packet from destination to source of given ID.
         *   history[Packet::id()] = metadata;
         *
         * Metadata must be removed from history when its ACK is send to its origin.
         * Metadata and Packet must release memory when removing from history.
         *
         * @see Packet::id()
         */
        PacketsHistory packets_history;

        /**
         * Keeps map of local frame ID (used to follow Xbee StatusFrame) and Metadata.
         *
         * After receiving StatusFrame, Metadata must be remove from history_frames,
         * but DO NOT release memory - Metadata still lives in history.
         *
         * @see Metadata::frame_id
         */
        FramesHistory frames_history;

        //! History lock - must be used when accessing (for reading or writing) history or history_frames.
        std::recursive_mutex history_lock;

        static const size_t bit_set_size = 254;

        /**
         * Contains local frame IDs used to communicate with Xbee.
         *
         * Local frame IDs should be unique, to ensure proper interpretation of StatusFrame.
         */
        std::bitset<bit_set_size> id_occupation;

        //! ID occupation lock - must be used when distributing ids.
        std::recursive_mutex id_occupation_lock;

       public:
        /**
         * Add Metadata to packet and frame history
         *
         * @param meta Metadata* to insert
         */
        void add(Metadata* meta);

        /**
         * Erase packet and free metadata (with metadata->packet)
         *
         * @param packet packet to erase
         */
        void erase(Packet* packet);

        /**
         * Erase frame (do not free metadata or metadata->packet)
         *
         * @param frame frame to erase
         */
        void erase(Frame* frame);

        /**
         * Erase frame (do not free metadata or metadata->packet)
         *
         * @param frameId id of frame to erase
         */
        void erase_frame(uint8_t frameId);

        /**
         * Get metadata for packet (from source to origin).
         *
         * @param packet
         *
         * @return metadata or nullptr if packet does not exists in history
         */
        Metadata* meta(Packet* packet);

        /**
         * Get metadata for frame.
         *
         * @param frame
         *
         * @return metadata or nullptr if frame does not exists in history
         */
        Metadata* meta(Frame* frame) const;

        /**
         * Find next free ID for Xbee frame.
         * IDs are probably unique, after receiving StatusFrame
         * ID is released. However when every ID is occupied, random
         * ID is returned.
         *
         * @return ID for Xbee frame - greater then 0 (0x00 is no status ID).
         */
        uint8_t reserve_id();

        /**
         * Release ID for given frame
         *
         * @param frame StatusFrame to release
         */
        void release_id(Frame* frame);

        /**
         * Add packet to history and makes the packet watched for delivery.
         *
         * Packet is given local frame_id using History::reserve_id(), then it is
         * added to history with current path.
         *
         * If packet already exists in history, its frame_id is updated and path is added.
         *
         * If packet is new (not received from another hop), a new Packet::frame_id is given,
         * equal to local frame_id, otherwise Packet::frame_id is constant (it is set on first
         * node). It is so, because triplet of destination, source and frame_id makes unique
         * Packet::id() at given network state.
         *
         * @return Packet local frame ID (must be set)
         * @see Dispatcher::scan()
         * @see Dispatcher::deliver()
         */
        uint8_t watch(Packet* p, Path path);


        //! lock mutex
        inline void lock() {
          history_lock.lock();
        }
        //! unlock mutex
        inline void unlock() {
          history_lock.unlock();
        }
        //! @return frames history
        inline FramesHistory &frames() {
          return frames_history;
        }
        //! @return packets history
        inline PacketsHistory &packets() {
          return packets_history;
        }
      };
    }
  }
}
#endif
