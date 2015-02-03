#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "common.h"
#include "../../src/router/dispatcher.h"

using namespace PUT::CS;

/**
 * Calculated timeout should be greater then 0 and greater then sum of average delay.
 * It should be incresing function of path size.
 */
TEST(DispatcherTest, timeout) {
  XbeeRouting::Xbee xbee("/dev/null");
  XbeeRouting::Address self = 1;
  XbeeRouting::Driver driver;
  XbeeRouting::Network network(self);
  XbeeRouting::Dispatcher dispatcher(xbee, network, driver);

  const int pathSize = 7;
  uint16_t delays[pathSize] = { 10, 15, 43, 3, 53, 8, 17 };

  SET_EDGE_WITH_DELAY(network, 1, 2, 15, 1, 2, delays[0]);
  SET_EDGE_WITH_DELAY(network, 2, 3, 15, 1, 2, delays[1]);
  SET_EDGE_WITH_DELAY(network, 3, 4, 15, 1, 2, delays[2]);
  SET_EDGE_WITH_DELAY(network, 4, 5, 15, 1, 2, delays[3]);
  SET_EDGE_WITH_DELAY(network, 5, 6, 15, 1, 2, delays[4]);
  SET_EDGE_WITH_DELAY(network, 6, 7, 15, 1, 2, delays[5]);
  SET_EDGE_WITH_DELAY(network, 7, 8, 15, 1, 2, delays[6]);

  network.node(1)->mac = 1;
  network.node(2)->mac = 1;
  network.node(3)->mac = 1;
  network.node(4)->mac = 1;
  network.node(5)->mac = 1;
  network.node(6)->mac = 1;
  network.node(7)->mac = 1;
  network.node(8)->mac = 1;

  XbeeRouting::Packet packet;
  packet.type = XbeeRouting::Packet::Type::Data;
  packet.length = 0;
  packet.source = self;
  std::chrono::steady_clock::time_point prevTimeout = std::chrono::steady_clock::now();

  for (int i = 0; i < pathSize; i++) {
    packet.destination = i + 2;
    XbeeRouting::Path path = network.path(self, XbeeRouting::Address(i + 2), XbeeRouting::Path());

    ASSERT_EQ(i + 1, (int)path.size());

    std::chrono::steady_clock::time_point timeout = dispatcher.timeout(&packet, path);
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    uint32_t delaySum = 0; for (int j = 0; j <= i; j++) delaySum += delays[j];

    EXPECT_THAT(timeout, testing::Ge(prevTimeout));

    EXPECT_THAT(std::chrono::duration_cast<std::chrono::milliseconds>(timeout - now).count(), testing::Ge(2 * delaySum));
    prevTimeout = timeout;
  }
}
