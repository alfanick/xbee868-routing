#include "packet.h"

#include <stdlib.h>
#pragma pack(push)
#pragma pack(1)

namespace PUT {
  namespace CS {
    namespace XbeeRouting {
      Packet::Packet(Packet* p, Address src, Address stat) : type(Type::Ack) {
        packet_id = p->packet_id;
        destination = p->visited.empty() ? p->source : p->visited.back();
        origin = p->source;
        source = src;
        length = 0;
        status = stat;
      }

      Packet::~Packet() {
        if (this == nullptr)
          return;

        switch (type) {
          case Type::Internal:
            delete data.frame;
            data.frame = nullptr;
            break;

          case Type::Data:

            // case Type::Jumbo:
            if (length > 0)
              free(data.content);

            data.content = nullptr;
            length = 0;
            break;

          case Type::Graph:
            if (length > 0)
              free(data.edges);

            data.edges = nullptr;
            length = 0;
            break;

          case Type::Ack:
            if (length > 0)
              delete data.parameters;

            data.parameters = nullptr;
            length = 0;

          default:
            break;
        }
      }

      PacketId Packet::id() const {
        if (type == Packet::Type::Ack)
          return (((uint32_t)source) << 16) | (((uint32_t)origin) << 8) | packet_id;
        else
          return (((uint32_t)destination) << 16) | (((uint32_t)source) << 8) | packet_id;
      }

      Packet::Type Packet::from_frame(Frame* frame) {
        uint8_t p = 0;
        uint8_t l = 0;
        uint8_t visited_count = 0;
        mac = frame->data.receive.mac;

        type = (Packet::Type)(frame->data.receive.data[p++]);
        l = frame->length;

        switch (type) {
          case Type::Data:
            destination = frame->data.receive.data[p++];
            source = frame->data.receive.data[p++];
            packet_id = frame->data.receive.data[p++];
            port = frame->data.receive.data[p++];
            visited_count = frame->data.receive.data[p++];

            for (int i = 0; i < visited_count; i++)
              visited.push_back(frame->data.receive.data[p++]);

            length = l - p;
            DLOG(INFO) << "Deserializing data frame, content length is " << (int) length;
            data.content = (uint8_t*)malloc(length * sizeof(uint8_t));
            memcpy(data.content, frame->data.receive.data + p, length);
            p += length;
            break;

          case Type::Ack:
            destination = frame->data.receive.data[p++];
            source = frame->data.receive.data[p++];
            packet_id = frame->data.receive.data[p++];

            origin = frame->data.receive.data[p++];
            status = frame->data.receive.data[p++];

            length = (l - p) / 4;
            data.parameters = (RemoteParameters*)malloc(length * sizeof(RemoteParameters));

            for (int i = 0; i < length; i++) {
              data.parameters[i].hop = frame->data.receive.data[p++];
              data.parameters[i].delay = ((uint16_t)frame->data.receive.data[p++]) << 8;
              data.parameters[i].delay |= frame->data.receive.data[p++];
              data.parameters[i].errors = frame->data.receive.data[p] >> 4;
              data.parameters[i].retries = (frame->data.receive.data[p++] << 4) >> 4;
            }

            p += length;
            break;

          case Type::NodeBroadcast:
            data.address = frame->data.receive.data[p++];
            break;

          case Type::EdgeDrop:
            length = 2;
            data.edge[0] = frame->data.receive.data[p++];
            data.edge[1] = frame->data.receive.data[p++];
            break;

          case Type::Graph:
            length = (l - 1) / 2;
            data.edges = (Edge*)malloc(length * sizeof(Address) * 2);
            memcpy(data.edges, frame->data.receive.data + p, length * 2);
            p += length * 2;
            break;

          default:
            LOG(FATAL) << "Trying to deserialize frame of unknown type";
            break;
        }

        return type;
      }

      Frame* Packet::to_frame() {
        Frame* frame = new Frame(Frame::Type::Transmit);

        unsigned char d[256];
        uint8_t l = 0;

        d[l++] = (unsigned char)type;

        switch (type) {
          case Type::Data:
            d[l++] = destination;
            d[l++] = source;
            d[l++] = packet_id;
            d[l++] = port;
            d[l++] = visited.size();

            for (uint8_t n : visited)
              d[l++] = n;

            DLOG(INFO) << "Serializing data frame, content length is " << (int) length;
            memcpy(d + l, data.content, length);
            l += length;
            break;

          case Type::Ack:
            d[l++] = destination;
            d[l++] = source;
            d[l++] = packet_id;
            d[l++] = origin;
            d[l++] = status;

            for (int i = 0; i < length; i++) {
              d[l++] = data.parameters[i].hop;
              d[l++] = data.parameters[i].delay >> 8;
              d[l++] = (data.parameters[i].delay << 8) >> 8;
              d[l++] = (data.parameters[i].errors << 4) | ((data.parameters[i].retries << 4) >> 4);
            }

            // FOR POSTERITY: my fault ALJ
            // l += 4 * length;
            break;

          case Type::NodeBroadcast:
            d[l++] = data.address;
            break;

          case Type::EdgeDrop:
            d[l++] = data.edge[0];
            d[l++] = data.edge[1];
            break;

          case Type::Graph:
            memcpy(d + l, data.edges, length * 2);
            l += length * 2;
            break;

          default:
            LOG(FATAL) << "Trying to serialize frame of unknown type";
            break;
        }

        frame->length = l;
        frame->data.transmit.data = (unsigned char*)malloc(frame->length * sizeof(unsigned char));
        memcpy(frame->data.transmit.data, d, frame->length);

        return frame;
      }

      Frame* Packet::to_frame(uint8_t id, uint64_t mac, uint16_t network, uint8_t radius, uint8_t options) {
        Frame* frame = to_frame();

        frame->data.transmit.id = id;
        frame->data.transmit.mac = mac;
        frame->data.transmit.network = network;
        frame->data.transmit.radius = radius;
        frame->data.transmit.options = options;

        return frame;
      }
    }
  }
}
#pragma pack(pop)
