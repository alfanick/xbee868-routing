#include "dispatcher.h"
#include <limits>
#define UINT16_MAX std::numeric_limits<uint16_t>::max()

namespace PUT {
  namespace CS {
    namespace XbeeRouting {

      Dispatcher::Dispatcher(Xbee &x, Network &n, Driver &d) : network(n), xbee(x), self(n.self()), driver(d) {
        tick_threadRun.store(true);

        tick_thread = std::thread([this]() {
          THREAD_NAME("Tick");
          int outdated = 0;

          while (tick_threadRun.load()) {
            outdated = tick();

            std::this_thread::sleep_for(tick_sleep_time);
          }
        });
      }

      Dispatcher::~Dispatcher() {
        tick_thread.detach();
      }

      bool Dispatcher::deliver(Packet* packet) {
        Path path = network.path(self->address, packet->destination, packet->visited);

        if (path.empty()) {
          LOG(WARNING) << "Packet could NOT be delivered, no route exists";

          if (packet->source == self->address)
            driver.deliver_back(packet->destination, packet->port, packet->data.content, packet->length);

          return false;
        }

        uint8_t id = history.watch(packet, path);

        Frame* frame = packet->to_frame(id, network.mac(path.front()), self->network);
        xbee.send(frame);
        delete frame;

        return true;
      }

      bool Dispatcher::retransmit(Metadata* meta) {
        Packet* p = meta->packet;
        Path path = network.path(self->address, p->destination, p->visited);

        if (path.empty()) {
          LOG(WARNING) << "Packet could NOT be delivered, no route exists (retransmission)";

          if (p->source == self->address)
            driver.deliver_back(p->destination, p->port, p->data.content, p->length);

          return false;
        }

        LOG(WARNING) << "Retransmiting packet, old frame_id: " << meta->frame_id;

        history.lock();

        LOG(WARNING) << "retransmit: Removing frame from history";
        history.erase_frame(meta->frame_id);

        meta->path_history.push_back(path);
        meta->frame_id = history.reserve_id();
        meta->send_time = std::chrono::steady_clock::now();
        meta->check_timeout = false;

        Frame* frame = meta->packet->to_frame(meta->frame_id, network.mac(path.front()), self->network);
        xbee.send(frame);
        history.frames()[meta->frame_id] = meta;

        history.unlock();

        delete frame;

        return true;
      }

      bool Dispatcher::try_retransmit(Metadata* meta) {
        LOG(WARNING) << "try_retransmit";

        if (meta->retransmission_counter++ < Metadata::retransmission_max)
          return retransmit(meta);

        LOG(WARNING) << "try_retransmit2";

        //WARNING that max retries is reached and do something if source is self
        Packet* p = meta->packet;
        LOG(WARNING) << "-----max retransmitions reached----";
        if (p->source == self->address)
          driver.deliver_back(p->destination, p->port, p->data.content, p->length);

        return false;
      }

      bool Dispatcher::send(Packet* packet) {
        Frame* frame = packet->to_frame(0, network.mac(packet->destination), self->network);

        if (network.mac(packet->destination) == Frame::BROADCAST) {
          LOG(WARNING) << "Packet could NOT be delivered, because packet dest is not adjacent";

          if (packet->source == self->address)
            driver.deliver_back(packet->destination, packet->port, packet->data.content, packet->length);

          return false;
        }

        xbee.send(frame);
        delete frame;

        return true;
      }

      void Dispatcher::broadcast(Packet* packet) {
        packet->destination = 0;
        Frame* frame = packet->to_frame(0, Frame::BROADCAST, self->network);

        xbee.send(frame);
        delete frame;
      }

      int Dispatcher::tick() {
        int outdated = 0;

        history.lock();

        for (auto meta : history.packets()) {
          if (!meta.second->check_timeout || meta.second->timeout >= std::chrono::steady_clock::now())
            continue;

          outdated++;
          LOG(WARNING) << "outdated " << outdated;
          if (meta.second->packet->source != self->address || !try_retransmit(meta.second)) {
            //free memory from meta and packet
            LOG(WARNING) <<  "----------------tick::erase-------------------";
            history.erase_frame(meta.second->frame_id);
            history.erase(meta.second->packet);
            LOG(WARNING) <<  "----------------tick::after_erase-------------------";
          }

        }

        history.unlock();
        return outdated;
      }

      inline void Dispatcher::handle_data(Packet* packet) {
        if (packet->destination == self->address) {
          send_ack(packet, 0);
          LOG(WARNING) << "Data packet delivered, sending ACK for packet" << (int) packet->packet_id;
        }
      }

      inline void Dispatcher::handle_ack(Packet* packet) {
        Metadata* meta;
        Address previous;
        bool retry;

        history.lock();
        meta = history.meta(packet);

        previous = network.from_mac(packet->mac);

        for (int i = packet->length - 1; i >= 0; i--) {
          network.update(previous, packet->data.parameters[i].hop, packet->data.parameters[i].retries, packet->data.parameters[i].errors, packet->data.parameters[i].delay);
          LOG(INFO) << "Updating edge [ " << (int) previous << " -> " << (int) packet->data.parameters[i].hop << " ]";
          previous = packet->data.parameters[i].hop;
        }

        packet->length++;
        packet->data.parameters = (RemoteParameters*)realloc((void*)packet->data.parameters, packet->length * sizeof(RemoteParameters));
        packet->data.parameters[packet->length - 1] = meta->frame_status;

        retry = false;

        if (packet->origin != self->address) {
          packet->destination = meta->packet->visited.size() == 1
                                ? packet->origin
                                : *(history.meta(packet)->packet->visited.cend() - 2);

          LOG(INFO) << "Passing ACK to " << (int) packet->destination;
          send(packet);
        } else if (packet->status != 0) {
          retry = try_retransmit(meta);
        }

        if (!retry) {
          LOG(WARNING) << "handle ack: Removing packet from history";
          history.erase_frame(meta->frame_id);
          history.erase(packet);
          LOG(INFO) << "Removing packet from history";
        }

        history.unlock();
      }

      inline void Dispatcher::handle_internal(Packet* packet) {
        Frame* frame;
        Metadata* meta;

        frame = packet->data.frame;

        if (frame->type == Frame::Type::Status) {
          history.lock();
          meta = history.meta(frame);

/////
          if (!meta) {
            LOG(WARNING) << "handle_internal meta is NULL at begin";
            history.release_id(frame);
            history.unlock();
            return;
          }
          auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - meta->send_time).count();
          LOG(WARNING) << "millis: " << millis;

          meta->frame_status.hop = meta->path_history.back().front();
          meta->frame_status.delay = std::min((long long)meta->frame_status.delay + millis, (long long)UINT16_MAX);
          meta->frame_status.errors += (frame->data.status.status > 0);
          meta->frame_status.retries += frame->data.status.retries;

          LOG(INFO) << "Status for " << (int) frame->data.status.id << ":'" << (char*) meta->packet->data.content << "'"
                     << " - retries " << (int) frame->data.status.retries << ", errors " << (int) frame->data.status.status << ", time " << (int) millis << " ms";

          network.update(self->address, meta->path_history.back().front(), frame->data.status.retries, frame->data.status.status, millis);

          if (frame->data.status.status > 0) { // retransmit if error
            if (!try_retransmit(meta)) { //too many retransmisions

              if (packet->source != self->address) { // send ack with status == self->address
                send_ack(packet, self->address);
                LOG(INFO) << "Data packet not delivered, sending ACK with status: " << self->address << " for packet" << (int) packet->packet_id;
              }

              if (network.edge(self->address, meta->path_history.back().front())->antireliability() > Dispatcher::antireliability_trheshold)
                broadcast_edge_drop(self->address, meta->path_history.back().front());

              LOG(WARNING) << "handle internal: Removing packet from history";
              history.erase(frame);
              history.erase(meta->packet);
            }
          } else {
            meta->timeout = timeout(meta->packet, meta->path_history.back());
            meta->check_timeout = true;

            LOG(WARNING) << "handle internal: Removing frame from history, status =0";
            history.erase(frame);
          }

          history.release_id(frame);

          history.unlock();
        }
      }

      void Dispatcher::scan(Packet* packet) {
        switch (packet->type) {
          case Packet::Type::Data:
            handle_data(packet);
            break;

          case Packet::Type::Ack:
            handle_ack(packet);
            break;

          case Packet::Type::Internal:
            handle_internal(packet);
            break;

          default:
            break;
        }
      }


      std::chrono::steady_clock::time_point Dispatcher::timeout(Packet* packet, Path &path) {
        Timeout t = 0;

        //see [Inz] Timeouts math google doc
        uint32_t edges_sum = 0;

        Address a = self->address;

        for (auto b : path) {
          Parameters* p = network.edge(a, b);
          edges_sum += p->delay * (1 + (p->retries / (p->good - p->retries > 0 ? p->good - p->retries : 1)));
          a = b;
        }

        t = (edges_sum + Dispatcher::tv * path.size()) * Dispatcher::c * Metadata::retransmission_max;

        if (packet->source != self->address)  // increasing timeout on non-source nodes (needed when ack of packet doesn't come back through current node)
          t *= Dispatcher::timeout_multiplier;

        LOG(WARNING) << "timeout calculated: " << (int) t;
        return (std::chrono::steady_clock::now() + std::chrono::milliseconds(t));
      }


      void Dispatcher::send_ack(Packet* packet, Address status) {
        Packet* response = new Packet(packet, self->address, status);
        send(response);

        delete response;
      }

      void Dispatcher::broadcast_edge_drop(Address a, Address b) {
        LOG(WARNING) << "Broadcasting EdgeDrop from " << self->address << " for edge " << a << "->" << b;

        Packet* p = new Packet(Packet::Type::EdgeDrop);
        p->data.edge[0] = a;
        p->data.edge[1] = b;

        broadcast(p);
        delete p;
      }
    }
  }
}
