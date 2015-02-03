#ifndef PUT_RADIO_NETWORK_H
#define PUT_RADIO_NETWORK_H

#include <stdlib.h>
#include <stdint.h>
#include <vector>
#include <deque>
#include <algorithm>
#include <mutex>
#include <cmath>

#include "../radio.h"
#include "node.h"
#include "packet.h"
#include <unordered_map>

namespace PUT {
  namespace CS {
    namespace XbeeRouting {
      /**
       * Map of map  type shortcut.
       *
       * Use unordered map (since C++11) - it is faster then std::map
       * (which is typically implemented as binary search trees)
       */
      template <class K, class V>
      class GraphMap : public std::unordered_map<K, std::unordered_map<K, V>> { };

      /**
       * Network edge parameters.
       *
       * Parameters are based on real measures, got from StatusFrame
       * and ACK packets.
       *
       * Parameters on single edge are constantly changing, summing every
       * parameter. Based on that edge "quality measure" is calculated
       * and used as cost in Network::path() algorithm.
       */
      struct Parameters {
        //! Number of delivered packages
        uint32_t good = 0;

        //! Number of undelivered packages
        uint32_t errors = 0;

        //! Number of retries
        uint32_t retries = 0;

        // Delay on edge [ms]
        uint16_t delay = 10;

        /**
         * Reliability measure - used as cost in Network::path()
         *
         * Lower the number, reliability of the edge is bigger.
         *
         * @see Network::path()
         */
        float antireliability() const;

        /**
         * Comparision operator based on antireliability().
         */
        static bool compare(Parameters* const &a, Parameters* b) {
          return a->antireliability() < b->antireliability();
        }
      };

      /**
          * XbeeGraphMap structure typedef.
          *
          * Now it is using pointers but insert/erase methods
          * can be override to implements symetric memory coppying.
          */
      using XbeeGraphMap = GraphMap<Address, Parameters*>;

      /**
       * Represents network state visible on self node.
       *
       * Network is made of nodes, which are connected with edges. Every edge
       * is described using Parameters. Edges are bidirectional and only single
       * edge connecting two nodes may exist in network.
       */
      class Network {
       private:
        //! Logical address to Node map
        std::map< Address, Node* > nodes;

        //! Self Node definition.
        Node* self_node;

        //! Adjacency map
        XbeeGraphMap neighbours;

        //! Must lock if changing graph
        std::mutex graphLock;

        //! Maximal address in the node - lower the better, used by Network::path()
        Address max_address = 0;
       public:
        /**
         * Construct network with self node.
         *
         * @param self self node address
         */
        Network(Address self);

        /**
         * Destroy network - releases memory used by nodes and edges.
         */
        ~Network();

        /**
         * Merges current network with the additional edges.
         *
         * If edge does not exist, it is created (so as corresponding nodes).
         *
         * @param edges Array of edges (usually taken from Packet::Type::Graph)
         * @param length Number of edges
         * @return True if any new edge added
         * @see Network::add_edge()
         */
        bool merge(Edge* edges, uint8_t length);

        /**
         * Creates new edge if not existing.
         *
         * If edge does not exist, it is created (so as corresponding nodes).
         *
         * @param a Source
         * @param b Destination
         * @return True if new edge added
         * @see Network::add_node()
         */
        bool add_edge(Address a, Address b);

        /**
         * Create new node if not existing.
         *
         * If node does not exists, it is created.
         *
         * @param a Address
         * @return True if new node created
         */
        bool add_node(Address a);

        /**
         * Check if there is an edge between two nodes.
         *
         * @param a Source
         * @param b Destination
         * @return True if edge does exist
         */
        bool adjacent(Address a, Address b) const;

        /**
         * Removes edge from the network.
         *
         * Removes edge from edges map and adjacency map.
         * If after removal, connected nodes have no more connections
         * the nodes are removed too.
         *
         * @param a Source
         * @param b Destination
         * @return True if edge is removed.
         */
        bool drop(Address a, Address b);

        /**
         * Get edge parameters.
         *
         * If edge is not existing, it is created and default parameters
         * are returned.
         *
         * @param a Source
         * @param b Destination
         * @return Parameters of edge (may be modified)
         */
        Parameters* edge(Address a, Address b);

        /**
         * Get node definition.
         *
         * If node is not existing, it is created with default definition.
         *
         * @param address Address
         * @return Node definition (may be modified)
         */
        Node* node(Address address);

        /**
         * Return self node.
         *
         * @return Self node.
         */
        Node* self() const;

        /**
         * Dumps every edge in the network to array of Edge.
         *
         * @param length Number of edges in the network
         * @return Edges in the network
         * @see Packet::Type::Graph
         */
        Edge* graph(uint8_t &length) const;

        /**
         * Finds MAC address of the logical Node.
         *
         * If MAC isn't defined, Frame::BROADCAST broadcast address
         * is returned.
         *
         * If node is adjacent to self and MAC address isn't Frame::BROADCAST,
         * you may assume that these two nodes can communicate.
         *
         * If node does not exist in the network, 0 is returned
         *
         * @param a Address
         * @return MAC address of node, 0 or Frame::BROADCAST
         */
        uint64_t mac(Address a) const;

        /**
         * Updates parameters of given edge.
         *
         * Retries and error are added to current edge parameters
         * (arithmetical sum is made). Parameters::good is incremented
         * if error is equal to 0x00.
         *
         * The method mantains parameters overflow - they would never
         * overflow, they will stop at UINT32_MAX if so would happen.
         *
         * @param a Source
         * @param b Destination
         * @param retries Number of retries
         * @param error Number of undelivered packets
         * @param delay Delay on edge (a,b) [ms]
         * @see RemoteParameters
         * @see Parameters
         * @see StatusFrame
         */
        void update(Address a, Address b, uint8_t retries, uint8_t error, uint16_t delay);

        /**
         * Find the most reliable path connecting two nodes, without visiting
         * already visited nodes if possible.
         *
         * Algorithm is based on modified Dijsktra shortest path algorithm
         * implemented using priority queue.
         *
         * The state of the graph is locked while algorithm is running.
         * Algorithm returns every hop till destination, without source, where
         * sum of the antireliability measure is lowest on the whole path.
         *
         * Path where sum of the antireliability measures on each edge is lowest,
         * is the most reliable path.
         *
         * Algorithm may return path with already visited node if there is no
         * other way to resolve the problem. Algorithm will ensure that
         * first hop is adjacent to source, however it may return a node with
         * no MAC address known, if there is no other path to destination.
         *
         * In perfect conditions (adjacent nodes have MAC known addresses
         * and visited nodes are not in the path) the algorithm will return
         * the very best, most actual path.
         *
         * If connecting source with destination, by any means neccessary,
         * is not possible, empty path will be returned.
         *
         * @param from Source address
         * @param to Destination address
         * @param visited Already visited Node addresses
         * @return The most reliable path
         * @see Router
         * @see Dispatcher::deliver()
         * @see Packet::Type::Data
         */
        Path path(Address from, Address to, Path visited);

        Address from_mac(uint64_t mac);
      };
    }
  }
}
#endif
