#include "history.h"

namespace PUT {
  namespace CS {
    namespace XbeeRouting {

      Metadata::~Metadata() {
        // packet should be deleted manually because of case in Dispatcher::tick(), when metadata have to be deleted, but pointer to packet should be valid
        //delete packet;
      }

      void History::erase(Packet* packet) {
        auto it = packets_history.find(packet->id());
        Metadata* tmp = it->second;

        packets_history.erase(it);

        if (packet == tmp->packet) {
          delete tmp->packet;
          packet = nullptr;
        } else
          delete tmp->packet;

        delete tmp;
      }

      void History::erase(Frame* frame) {
       LOG(WARNING)<<"erase";
        frames_history.erase(frame->data.status.id);
      }

      void History::erase_frame(uint8_t frameId) {
        LOG(WARNING)<<"erase_frame";
         frames_history.erase(frameId);
      }

      Metadata* History::meta(Packet* packet) {
        auto it = packets_history.find(packet->id());
        return it != packets_history.end() ? it->second : nullptr;
      }

      Metadata* History::meta(Frame* frame) const {
        auto it = frames_history.find(frame->data.status.id);
        return it != frames_history.end() ? it->second : nullptr;
      }

      void History::add(Metadata* meta) {
        packets_history[meta->packet->id()] = meta;
        frames_history[meta->frame_id] = meta;
      }

      uint8_t History::watch(Packet* packet, Path path) {
        lock();

        Metadata* meta = this->meta(packet);

        if (meta == nullptr)
          meta = new Metadata();

        DLOG(INFO) << "Watching packet, visited size:  " << packet->visited.size();
        meta->packet = packet;
        meta->path_history.push_back(path);
        meta->frame_id = reserve_id();

        if (packet->packet_id == 0) {
          packet->packet_id = meta->frame_id;
          DLOG(INFO) << "New frame id set to " << (int) packet->packet_id;
        }

        meta->packet_id = packet->id();

        add(meta);

        meta->send_time = std::chrono::steady_clock::now();

        unlock();
        return meta->frame_id;
      }

      uint8_t History::reserve_id() {
        uint8_t id;

        id_occupation_lock.lock();

        for (id = 0; id < bit_set_size; id++)
          if (!id_occupation.test(id))
            break;

        if (id == bit_set_size)
          id = rand() % bit_set_size;

        id_occupation.set(id);

        id_occupation_lock.unlock();

        return id + 1;
      }

      void History::release_id(Frame* frame) {
        id_occupation_lock.lock();
        id_occupation.reset(frame->data.status.id - 1);
        id_occupation_lock.unlock();
      }

    }
  }
}
