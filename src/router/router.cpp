#include "router.h"

#include <unistd.h>
#include <chrono>
#include <thread>

namespace PUT {
  namespace CS {
    namespace XbeeRouting {
      Router::Router(char* serial_port, uint8_t address): xbee(serial_port), network(address), self(network.self()), driver(), dispatcher(xbee, network, driver) {
        uint8_t length;
        char* result;
        uint64_t a = 0;

        // Node Identifier
        result = xbee.get("NI", length);
        self->name = std::string(result);
        free(result);

        // Network ID
        result = xbee.get("ID", length);
        memcpy(&a, result, 2);
        self->network = a;
        free(result);

        // Serial Number Low
        result = xbee.get("SL", length);
        a = 0;
        memcpy(&a, result, 4);
        self->mac = a << 32;
        free(result);

        // Serial Number High
        result = xbee.get("SH", length);
        a = 0;
        memcpy(&a, result, 4);
        self->mac |= a;
        free(result);

        unsigned char t[1];

        t[0] = 0;
        xbee.command("PL", t, 1);

        t[0] = 1;
        xbee.command("MT", t, 1);

        driver.self([this](Address destination, uint8_t port, Address source, uint8_t* data, size_t length) {
          if (destination != Driver::SELF) {
            Packet* packet = new Packet(Packet::Type::Data);
            packet->source = (source == Driver::SELF) ? self->address : source;
            packet->destination = destination;
            packet->port = port;
            packet->length = length;
            packet->data.content = (uint8_t*)malloc(packet->length);
            memcpy(packet->data.content, data, length);
            dispatcher.deliver(packet);
          }
        });

        nodeBroadcasterRun.store(true);

        nodeBroadcaster = std::thread([this]() {
          THREAD_NAME("NodeBroadcast");
          Packet packet(self->address);

          while (nodeBroadcasterRun.load()) {
            dispatcher.broadcast(&packet);

            std::this_thread::sleep_for(std::chrono::seconds(15)); //! TODO
          }
        });

        heartbeatDetector = std::thread([this]() {
          THREAD_NAME("HeartbeatDetector");

          while (true) {
            auto last = std::chrono::steady_clock::now() - std::chrono::seconds(20);
            auto adjacent = network.neighbours.find(self->address);
            std::vector<Address> broken;

            if (adjacent != network.neighbours.end())
              for (auto& neighbour : adjacent->second)
                if (network.node(neighbour.first)->last_tick < last)
                  broken.push_back(neighbour.first);

            for (Address broken_node : broken) {
              network.drop(self->address, broken_node);

              Packet* p = new Packet(Packet::Type::EdgeDrop);
              p->data.edge[0] = self->address;
              p->data.edge[1] = broken_node;

              dispatcher.broadcast(p);
              delete p;
            }

            std::this_thread::sleep_for(std::chrono::seconds(3));
          }
        });

      }

      Router::~Router() {
      }

      Packet* Router::receive() {
        Frame* frame = xbee.receive();
        Packet* packet = new Packet();

        if (frame->type == Frame::Type::Receive) {
          packet->from_frame(frame);

          delete frame;
        } else {
          packet->type = Packet::Type::Internal;
          packet->length = frame->length;
          packet->data.frame = frame;
        }

        return packet;
      }

      void Router::process() {
        Packet* packet = receive();
        Packet* response;

        dispatcher.scan(packet);

        switch (packet->type) {
          case Packet::Type::Data:

            // route to next hop or send to redis if local or broadcast
            if (packet->destination == self->address || packet->destination == 0) {
              driver.deliver(packet->source, Driver::SELF, packet->port, packet->data.content, packet->length);

              DLOG(INFO) << "Received data: " << (char*) packet->data.content << " from " << (int) packet->source;
              std::stringstream ss;
              ss << "Data route was " << (int) packet->source << " ";
              // while (!packet->visited.empty()) {
              // ss << (int) packet->visited.front() << " ";
              // packet->visited.pop_front();
              // }
              // ss << (int) packet->destination;
              // DLOG(INFO) << ss.str();

              delete packet;

            } else {
              DLOG(INFO) << "Received data packet for routing from " << (int) packet->source << " to " << (int) packet->destination;
              // visited myself
              packet->visited.push_back(self->address);

              // next hop! assuming packet is deliver()
              dispatcher.deliver(packet);
            }

            break;

          case Packet::Type::Ack:
            for (int i = 0; i < packet->length; i++) {
              DLOG(INFO) << (int) packet->data.parameters[i].hop
                         << ": errors " << (int) packet->data.parameters[i].errors
                         << ", retries " << (int) packet->data.parameters[i].retries
                         << ", lag " << (int) packet->data.parameters[i].delay
                         << " ms";
            }

            delete packet;
            break;

          case Packet::Type::Internal:
            delete packet;
            break;

          case Packet::Type::NodeBroadcast:
            // add node if not adjacent
            if (network.node(packet->data.address)->mac == 0) {
              heartbeat();

              network.node(packet->data.address)->mac = packet->mac;

              network.add_edge(packet->data.address, self->address);

              response = new Packet(Packet::Type::Graph);
              response->data.edges = network.graph(response->length);

              dispatcher.broadcast(response);

              delete response;
            }

            network.node(packet->data.address)->last_tick = std::chrono::steady_clock::now();

            delete packet;

            break;

          case Packet::Type::EdgeDrop:
            if (network.drop(packet->data.edge[0], packet->data.edge[1])) {
              DLOG(INFO) << "Edge " << packet->data.edge[0] << "->" << packet->data.edge[1] << "dropped at node: " << self->address;
              DLOG(INFO) << "Broadcasting EdgeDrop from " << self->address << " for edge " << packet->data.edge[0] << "->" << packet->data.edge[1];
              dispatcher.broadcast(packet);
            }

            delete packet;
            break;

          case Packet::Type::Graph:
            if (network.merge(packet->data.edges, packet->length)) {
              response = new Packet(Packet::Type::Graph);
              response->data.edges = network.graph(response->length);

              dispatcher.broadcast(response);

              delete response;
            }

            delete packet;
            break;

          default:
            LOG(FATAL) << "Processing unknown packet type, aborting ...";
            delete packet;
        }
      }

      void Router::heartbeat() {
        Packet packet(self->address);
        dispatcher.broadcast(&packet);
      }

      void Router::run(char* serial_port, uint8_t address) {
        THREAD_NAME("Router");
        Router router(serial_port, address);

        printf("ROUTER RUN\r\n");
        fflush(stdout);

        while (true) {
          router.process();
        }
      }
    }
  }
}
