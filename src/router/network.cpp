#include "network.h"
#include <deque>
#include <fstream>
#include <iostream>
#include <queue>
#include <vector>
#include <limits>
#define UINT32_MAX std::numeric_limits<uint32_t>::max()

namespace PUT {
  namespace CS {
    namespace XbeeRouting {
      float Parameters::antireliability() const {
        return (retries * (errors + 1)) / float(good + 1);
      }

      Network::Network(Address self) {
        this->self_node = node(self);
        this->self_node->self = true;
      };

      Network::~Network() {
        graphLock.lock();

        for (auto &kv : nodes) {
          delete kv.second;
        }

        nodes.clear();

        for (auto &m1 : neighbours) {
          for (auto &m2 : m1.second) {
            if (m1.first < m2.first)
              delete m2.second;
          }

          m1.second.clear();
        }

        neighbours.clear();

        graphLock.unlock();
      }

      bool Network::merge(Edge* edges, uint8_t length) {
        bool dirty = false;

        DLOG(INFO) << "Merging two network graphs";

        for (int i = 0; i < length; i++) {
          dirty |= add_edge(edges[i][0], edges[i][1]);
        }

        DLOG(INFO) << "Finished merging";

        return dirty;
      }

      bool Network::add_edge(Address a, Address b) {
        if (a == b || a == 0 || a == 255 || b == 0 || b == 255)
          return false;

        bool dirty = false;

        add_node(a);
        add_node(b);

        graphLock.lock();
        auto it_a = neighbours.find(a);

        if (it_a == neighbours.end()) {
          dirty = true;
          it_a = neighbours.emplace(a, XbeeGraphMap::mapped_type()).first;
        }

        auto it_b = neighbours.find(b);

        if (it_b == neighbours.end()) {
          dirty = true;
          it_b = neighbours.emplace(b, XbeeGraphMap::mapped_type()).first;
        }

        auto it = it_a->second.find(b);

        if (it == it_a->second.end()) {
          dirty = true;
          Parameters* p = new Parameters();

          it_a->second[b] = p;
          it_b->second[a] = p;

          // DO NOT TOUCH, ever!
          printf("  EDGE %d %d\n", a, b);
          fflush(stdout);
        }

        graphLock.unlock();

        return dirty;
      }

      Parameters* Network::edge(Address a, Address b) {
        add_edge(a, b);

        return neighbours[a][b];
      }

      void Network::update(Address a, Address b, uint8_t retries, uint8_t error, uint16_t delay) {
        Parameters* parameters = edge(a, b);

        parameters->retries = std::min(parameters->retries + retries, UINT32_MAX);
        parameters->errors = std::min(parameters->errors + (error > 0), UINT32_MAX);
        parameters->good = std::min(parameters->good + (error == 0), UINT32_MAX);
        parameters->delay = (uint16_t)round((double(parameters->delay) + double(delay)) / 2);

        printf("UPDATE %d %d - retries %d, errors %d, good %d, delay %d, antireliability %f\n", a, b, parameters->retries, parameters->errors, parameters->good, parameters->delay, parameters->antireliability());
        fflush(stdout);
      }

      bool Network::add_node(Address a) {
        bool dirty = false;

        if (a == 0 || a == 255)
          return false;

        auto it = nodes.find(a);

        if (it == nodes.end()) {
          if (a > max_address)
            max_address = a;

          dirty = true;
          graphLock.lock();
          nodes[a] = new Node();
          nodes[a]->address = a;
          nodes[a]->mac = 0;
          graphLock.unlock();

          DLOG(INFO) << "Adding node " << (int) a;
        }

        return dirty;
      }

      Node* Network::node(Address a) {
        add_node(a);

        return nodes.find(a)->second;
      }

      uint64_t Network::mac(Address a) const {
        auto it = nodes.find(a);

        if (it == nodes.end())
          return 0;

        return it->second->mac != 0 ? it->second->mac : Frame::BROADCAST;
      }

      // FIXME make it O(1)
      Address Network::from_mac(uint64_t mac) {
        for (auto &node : nodes) {
          if (node.second->mac == mac) {
            return node.second->address;
          }
        }

        return 0;
      }

      const float INFINITE_DISTANCE = 9999999;

      Path Network::path(Address from, Address to, Path visited) {
        DLOG(INFO) << "Finding path from " << (int)from << " to " << (int) to;

        graphLock.lock();

        std::vector<float> distance(max_address + 1, INFINITE_DISTANCE);
        std::vector<Address> previous(max_address + 1, 0);
        std::priority_queue< Destination, std::vector<Destination>, std::greater<Destination> > queue;
        Address next_node, current_node;
        float next_distance, current_distance;
        Path path;

        distance[from] = 0;
        queue.push(Destination(0.0f, from));

        while (!queue.empty()) {
          current_node = queue.top().second;
          current_distance = queue.top().first;
          queue.pop();

          if (current_distance <= distance[current_node]) {
            auto adjacent = neighbours[current_node];

            for (auto &nb : adjacent) {
              next_node = nb.first;
              next_distance = 0;

              next_distance += (current_node == from && nodes[next_node]->mac == 0) ? INFINITE_DISTANCE : 0;
              next_distance += (std::find(visited.begin(), visited.end(), next_node) != visited.end()) ? INFINITE_DISTANCE : 0;

              if (distance[next_node] > (next_distance += (distance[current_node] + nb.second->antireliability()))) {
                distance[next_node] = next_distance;
                previous[next_node] = current_node;
                queue.push(Destination(next_distance, next_node));
              }
            }
          }
        }

        graphLock.unlock();

        current_node = to;

        while (previous[current_node] != 0) {
          path.push_front(current_node);
          current_node = previous[current_node];
        }

        return path;
      }

      bool Network::adjacent(Address a, Address b) const {
        auto it_a = neighbours.find(a);

        return it_a != neighbours.end() && it_a->second.find(b) != it_a->second.end();
      }

      bool Network::drop(Address a, Address b) {
        bool dirty = false;
        graphLock.lock();

        auto it_a = neighbours.find(a);

        if (it_a != neighbours.end()) {
          dirty = true;
          auto it_ab = it_a->second.find(b);

          if (it_ab != it_a->second.end()) {
            delete it_ab->second;
            it_a->second.erase(it_ab);
          }
        }

        auto it_b = neighbours.find(b);

        if (it_b != neighbours.end()) {
          dirty = true;
          auto it_ba = it_b->second.find(a);

          if (it_ba != it_b->second.end()) {
            it_b->second.erase(it_ba);
          }
        }

        if (it_a->second.empty()) {
          nodes.erase(a);

          if (a == max_address)
            max_address--;

          DLOG(INFO) << "Dropping node " << (int) a;
        }

        if (it_b->second.empty()) {
          nodes.erase(b);
          DLOG(INFO) << "Dropping node " << (int) b;
        }

        graphLock.unlock();

        return dirty;
      }

      Node* Network::self() const {
        return self_node;
      }

      Edge* Network::graph(uint8_t &length) const {
        Edge* e = (Edge*)malloc(length * 2 * sizeof(Address));

        int i = 0;

        for (auto &m1 : neighbours) {
          for (auto &m2 : m1.second) {
            if (m1.first < m2.first) {
              e[i][0] = m1.first;
              e[i][1] = m2.first;
              i++;
            }
          }
        }

        length = i; //TODO what when delete and i< real_length!

        return e;
      }

    }
  }
}
