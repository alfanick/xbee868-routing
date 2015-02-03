#ifndef PUT_RADIO_ROUTER_H
#define PUT_RADIO_ROUTER_H

#include <string>
#include <map>
#include <thread>


#include "../radio.h"

#include "xbee.h"
#include "node.h"
#include "packet.h"
#include "network.h"
#include "dispatcher.h"
#include "../driver/driver.h"

namespace PUT {
  namespace CS {
    namespace XbeeRouting {
      /**
       * Network Router.
       *
       * Router creates instance of Xbee radio and redirects packets to local Redis
       * or to other Nodes. Every node is identified by unique 8 byte Address
       * (values from 1-255). Address 0 represents any node and its used in broadcasting
       * to prevent from further routing.
       *
       * Any node may join or leave network at any time. Packets must delivered to destination
       * or an information about error must be received. After packet delivery, an acknowledge
       * Packet (Packet::Type::Ack) is sent.
       *
       * There are four tasks for whole network:
       *
       *   1. Discover network topology
       *
       *      To join the network, Node broadcasts Packet::Type::NodeBroadcast,
       *      if any other Node receives this broadcast it broadcasts new NodeBroadcast for
       *      itself. An Edge (and Node) from received Node to itself is added, unless
       *      already in the Network. If the edge was added, every Edge in the Network
       *      is broadcasted as Packet::Type::Graph.
       *
       *      When Packet::Type::Graph is received, edges from the packet are merged
       *      with local Network graph. If it has changed, the new Packet::Type::Graph
       *      is broadcasted with current Network grpah.
       *
       *
       *   2. Maintain network topology and state
       *
       *      When Packet::Type::EdgeDrop is received, the Edge is removed from the
       *      Network. If it was removed successfully, the EdgeDrop is broadcasted.
       *      This operation allows to remove faulty edges in whole Network. If in the graph
       *      exists a Node without any edge, it should be removed locally.
       *
       *      Once in a while, a heartbeat is broadcasted. The Packet::Type::NodeBroadcast
       *      is broadcasted and procedure similiar to 1) is happening in the Network.
       *      This allows rediscovering Node if it was dropped (in case of Packet::Type::EdgeDrop).
       *
       *      With every Packet::Type::Ack Parameters of Edge are updated. For every Edge
       *      in the Ack, network parameters are increased by values from Ack (so antireliability
       *      measure is updated too).
       *
       *
       *   3. Deliver packets from source to destination
       *
       *      When Packet::Type::Data is received, unless Packet::destination is self, Packet
       *      is sent (well, Dispatcher::deliver()) to next Node in the path. If Packet
       *      reaches its destination it is published using Redis and Packet::Type::Ack is sent.
       *
       *      Path is obtained using Network::path() - Dijkstra shortest path algorithm.
       *      For every edge in the network an antireliability measure is calculated based on
       *      parameters such as number of packets delivered, undelivered and retried. With
       *      every Packet::Type::Ack graph parameters are updated, so path is always the
       *      most reliable, based on latest local graph state.
       *
       *      On every intermediate Node, Packet::visited list is updated. When finding the
       *      most reliable path, the path tries to avoid already visited nodes (in order to
       *      avoid cycles).
       *
       *      When StatusFrame is received, its parameters are stored in Dispatcher::history.
       *      If StatusFrame or Packet::Type::Ack is received with error and the Packet is
       *      undelivered, graph is updated and Packet is routed once again (Packet may be
       *      rerouted 3 times at most). If Packet is still undelivered, Ack with error is sent.
       *
       *      If no Ack is received in given timeout, Ack with error is sent.
       *
       *
       *   4. Acknowledgement packets
       *
       *      Acknowledgement packets are sent from destination to source, by exactly the same
       *      path the coresponding packet was delivered to the destination. The Ack packet
       *      contains parameters of every edge on the path, which are used to update graph on
       *      intermediate and source nodes. The Ack packets contains information about last
       *      node which couldn't deliver the packet.
       *
       *      When Packet::Type::Data is delivered to Packet::destination, empty Packet::Type::Ack
       *      is sent to last visited Node.
       *
       *      When Packet::Type::Ack is received and corresponding packet was delivered (that is
       *      when Packet::status is equal to 0), edge parameters from Dispatcher::history are
       *      appended to current Ack. Unless Ack reaches packet origin, it is sent to next hop
       *      (based on Packet::visited from Dispatcher::history).
       *
       *      When Ack is received, but packet was undelivered (Packet::status is equal to Node
       *      where error was found), according to 3) packet delivery is repeated.
       *
       *
       * Data which destination is self, are sent to Redis channel and are available to local
       * services via XbeeRouting::Driver. Data which must be delivered to other nodes, are read from
       * specific Redis channel as documented in Driver.
       *
       * @see Metadata
       * @see Dispatcher
       * @see Packet
       * @see Driver
       */
      class Router {
       private:
        /**driver
         * Xbee radio definition.
         *
         * Router creates an instance of Xbee radio. There should be only one Xbee radio
         * on one Node.
         */
        Xbee xbee;

        /**
         * Network graph definition.
         *
         * Router creates simple Network graph and adds self Node.
         */
        Network network;

        /**
         * Self Node definition.
         *
         * Required parameters (MAC address and network) are obtained
         * directly from connected Xbee module.
         */
        Node* self;

        /**
         * Network driver.
         *
         * Interface to other local services using Redis as communication
         * medium
         */
        Driver driver;

        /**
         * Every Packet is sent throught Dispatcher. So as every Packet
         * received from Router::receive() is scanned Dispatcher::scan().
         *
         * Dispatcher always has access to Xbee and current Network graph.
         */
        Dispatcher dispatcher;

        /**
         * Node broadcaster thread.
         *
         * Once in a while self node is broadcasted to keep alive
         * the network.
         */
        std::thread nodeBroadcaster;

        /**
         * Node broadcaster status.
         *
         * Broadcasting is enabled when true.
         */
        std::atomic_bool nodeBroadcasterRun;
       public:
        /**
         * Creates new Router instance.
         *
         * Router connects to Xbee and create empty Network graph. To obtain parameters
         * needed to initialize self, AT commands are sent to Xbee (NI - Node Identifier,
         * ID - Network ID, SL/SH - MAC address).
         *
         * Temporarely power level is set to the lowest, number of broadcast retransmissions
         * to single and console is made.
         *
         * Redis manager will be initialized here.
         *
         * @param serial_port Serial port path
         * @param address Node address
         */
        Router(char* serial_port, uint8_t address);

        /**
         * Destroys router.
         *
         * Stops Dispatcher and Xbee radio connection.
         * Deletes network. EdgeDrop should be send here before
         * destroying, to network graph about changes.
         *
         * @see Dispatcher::~Dispatcher()
         */
        ~Router();

        /**
         * Receives frame and parses it into Packet. Blocking.
         *
         * If Frame::Type::Receive, the Packet is simply restored from
         * Frame. Otherwise, Packet::Type::Internal is created.
         *
         * @return Parsed packet
         * @see Packet::from_frame()
         */
        Packet* receive();

        /**
         * Receive and process Packet.
         *
         * Router::process() waits for incoming Packet using Router::receive(),
         * after that Packet is send to Dispatcher by Disptacher::scan().
         *
         * The network topology discovery and maintanance is done in here.
         * Routing, that is delivering to next hop, based on Dispatcher::delivery()
         * is done in here.
         *
         * Discovery protocol is implemented as in Router description.
         *
         * If received Packet destination is self, packet is send to appropriate
         * Redis channel (TODO).
         *
         * @see Router
         * @see Dispatcher::scan()
         * @see Dispatcher::deliver()
         */
        void process();

        /**
         * Send NodeBroadcast to inform about yourself.
         *
         * Heartbeat is send upon start of the router and once in a while
         * to ensure freshness of the Network graph, as described in Router
         * description.
         *
         * @see Router
         */
        void heartbeat();

        /**
         * Blocking. Create router instance, start discovery and process frames.
         *
         * @param serial_port Serial port device path
         * @param address Logical address of the Node
         */
        static void run(char* serial_port, uint8_t address);
      };
    }
  }
}
#endif
