#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "common.h"
#include "../../src/router/network.h"

using namespace PUT::CS;

/**
 * Try to add_node multiple times and check if you can find it
 */
TEST(NetworkTest, add_node) {
  XbeeRouting::Network network(1);

  ASSERT_TRUE(network.add_node(XbeeRouting::Address(2)));
  ASSERT_TRUE(network.add_node(XbeeRouting::Address(3)));
  ASSERT_TRUE(network.add_node(XbeeRouting::Address(254)));
  ASSERT_TRUE(network.add_node(XbeeRouting::Address(10)));
  ASSERT_TRUE(network.add_node(XbeeRouting::Address(231)));
  ASSERT_TRUE(network.add_node(XbeeRouting::Address(11)));
  ASSERT_TRUE(network.add_node(XbeeRouting::Address(12)));
  ASSERT_TRUE(network.add_node(XbeeRouting::Address(128)));

  ASSERT_FALSE(network.add_node(XbeeRouting::Address(1)));

  ASSERT_EQ(network.node(XbeeRouting::Address(2))->address, XbeeRouting::Address(2));
  ASSERT_EQ(network.node(XbeeRouting::Address(3))->address, XbeeRouting::Address(3));
  ASSERT_EQ(network.node(XbeeRouting::Address(254))->address, XbeeRouting::Address(254));
  ASSERT_EQ(network.node(XbeeRouting::Address(10))->address, XbeeRouting::Address(10));
  ASSERT_EQ(network.node(XbeeRouting::Address(231))->address, XbeeRouting::Address(231));
  ASSERT_EQ(network.node(XbeeRouting::Address(11))->address, XbeeRouting::Address(11));
  ASSERT_EQ(network.node(XbeeRouting::Address(12))->address, XbeeRouting::Address(12));
  ASSERT_EQ(network.node(XbeeRouting::Address(128))->address, XbeeRouting::Address(128));
  ASSERT_EQ(network.node(XbeeRouting::Address(1))->address, XbeeRouting::Address(1));

  ASSERT_FALSE(network.add_node(XbeeRouting::Address(254)));
  ASSERT_FALSE(network.add_node(XbeeRouting::Address(3)));
  ASSERT_FALSE(network.add_node(XbeeRouting::Address(128)));
  ASSERT_FALSE(network.add_node(XbeeRouting::Address(10)));
  ASSERT_FALSE(network.add_node(XbeeRouting::Address(231)));
  ASSERT_FALSE(network.add_node(XbeeRouting::Address(11)));
  ASSERT_FALSE(network.add_node(XbeeRouting::Address(2)));
  ASSERT_FALSE(network.add_node(XbeeRouting::Address(12)));
  ASSERT_FALSE(network.add_node(XbeeRouting::Address(1)));

  ASSERT_EQ(network.node(XbeeRouting::Address(2))->address, XbeeRouting::Address(2));
  ASSERT_EQ(network.node(XbeeRouting::Address(3))->address, XbeeRouting::Address(3));
  ASSERT_EQ(network.node(XbeeRouting::Address(254))->address, XbeeRouting::Address(254));
  ASSERT_EQ(network.node(XbeeRouting::Address(10))->address, XbeeRouting::Address(10));
  ASSERT_EQ(network.node(XbeeRouting::Address(231))->address, XbeeRouting::Address(231));
  ASSERT_EQ(network.node(XbeeRouting::Address(11))->address, XbeeRouting::Address(11));
  ASSERT_EQ(network.node(XbeeRouting::Address(12))->address, XbeeRouting::Address(12));
  ASSERT_EQ(network.node(XbeeRouting::Address(128))->address, XbeeRouting::Address(128));
  ASSERT_EQ(network.node(XbeeRouting::Address(1))->address, XbeeRouting::Address(1));

  ASSERT_EQ(network.self()->address, XbeeRouting::Address(1));
}

/**
 * Create node via node method
 * Check node via node method
 */
TEST(NetworkTest, node) {
  XbeeRouting::Network network(21);

  ASSERT_EQ(network.node(XbeeRouting::Address(5))->address, XbeeRouting::Address(5));
  ASSERT_EQ(network.node(XbeeRouting::Address(254))->address, XbeeRouting::Address(254));
  ASSERT_EQ(network.node(XbeeRouting::Address(100))->address, XbeeRouting::Address(100));
  ASSERT_EQ(network.node(XbeeRouting::Address(77))->address, XbeeRouting::Address(77));

  ASSERT_EQ(network.node(XbeeRouting::Address(5))->address, XbeeRouting::Address(5));
  ASSERT_EQ(network.node(XbeeRouting::Address(254))->address, XbeeRouting::Address(254));
  ASSERT_EQ(network.node(XbeeRouting::Address(100))->address, XbeeRouting::Address(100));
  ASSERT_EQ(network.node(XbeeRouting::Address(77))->address, XbeeRouting::Address(77));

  ASSERT_EQ(network.self()->address, XbeeRouting::Address(21));
}

/**
 * Add forbidden address (node)
 * TODO add correct behivior
 */
TEST(NetworkTest, nodeForbiddenAddress) {
  XbeeRouting::Network network(16);

  EXPECT_FALSE(network.add_node(0));
  EXPECT_FALSE(network.add_node(255));

  EXPECT_FALSE(network.add_edge(XbeeRouting::Address(127), XbeeRouting::Address(0)));
  EXPECT_FALSE(network.add_edge(XbeeRouting::Address(0), XbeeRouting::Address(127)));
  EXPECT_FALSE(network.add_edge(XbeeRouting::Address(255), XbeeRouting::Address(6)));
  EXPECT_FALSE(network.add_edge(XbeeRouting::Address(0), XbeeRouting::Address(255)));

  EXPECT_FALSE(network.add_edge(XbeeRouting::Address(255), XbeeRouting::Address(255)));
  EXPECT_FALSE(network.add_edge(XbeeRouting::Address(0), XbeeRouting::Address(0)));
}

/**
 * Test add edge when nodes do not exists
 */
TEST(NetworkTest, add_edgeWhenNodesDoNotExists) {
  XbeeRouting::Network network(18);

  ASSERT_FALSE(network.adjacent(XbeeRouting::Address(4), XbeeRouting::Address(5)));
  ASSERT_FALSE(network.adjacent(XbeeRouting::Address(1), XbeeRouting::Address(154)));
  ASSERT_FALSE(network.adjacent(XbeeRouting::Address(18), XbeeRouting::Address(56)));

  ASSERT_TRUE(network.add_edge(XbeeRouting::Address(4), XbeeRouting::Address(5)));
  ASSERT_TRUE(network.add_edge(XbeeRouting::Address(1), XbeeRouting::Address(154)));
  ASSERT_TRUE(network.add_edge(XbeeRouting::Address(18), XbeeRouting::Address(56)));

  ASSERT_FALSE(network.add_node(XbeeRouting::Address(4)));
  ASSERT_FALSE(network.add_node(XbeeRouting::Address(5)));
  ASSERT_FALSE(network.add_node(XbeeRouting::Address(1)));
  ASSERT_FALSE(network.add_node(XbeeRouting::Address(154)));
  ASSERT_FALSE(network.add_node(XbeeRouting::Address(18)));
  ASSERT_FALSE(network.add_node(XbeeRouting::Address(56)));

  ASSERT_TRUE(network.adjacent(XbeeRouting::Address(4), XbeeRouting::Address(5)));
  ASSERT_TRUE(network.adjacent(XbeeRouting::Address(1), XbeeRouting::Address(154)));
  ASSERT_TRUE(network.adjacent(XbeeRouting::Address(18), XbeeRouting::Address(56)));

  ASSERT_NE(network.edge(XbeeRouting::Address(4), XbeeRouting::Address(5)), nullptr);
  ASSERT_NE(network.edge(XbeeRouting::Address(1), XbeeRouting::Address(154)), nullptr);
  ASSERT_NE(network.edge(XbeeRouting::Address(18), XbeeRouting::Address(56)), nullptr);

  ASSERT_FALSE(network.add_edge(XbeeRouting::Address(4), XbeeRouting::Address(5)));
  ASSERT_FALSE(network.add_edge(XbeeRouting::Address(1), XbeeRouting::Address(154)));
  ASSERT_FALSE(network.add_edge(XbeeRouting::Address(18), XbeeRouting::Address(56)));

  ASSERT_EQ(network.self()->address, XbeeRouting::Address(18));
}

/**
 * Test add edge when nodes exists
 */
TEST(NetworkTest, add_edgeWhenNodesExists) {
  XbeeRouting::Network network(1);

  ASSERT_FALSE(network.adjacent(XbeeRouting::Address(4), XbeeRouting::Address(5)));
  ASSERT_FALSE(network.adjacent(XbeeRouting::Address(1), XbeeRouting::Address(154)));
  ASSERT_FALSE(network.adjacent(XbeeRouting::Address(18), XbeeRouting::Address(56)));

  ASSERT_TRUE(network.add_node(XbeeRouting::Address(4)));
  ASSERT_TRUE(network.add_node(XbeeRouting::Address(5)));
  ASSERT_TRUE(network.add_node(XbeeRouting::Address(56)));

  ASSERT_TRUE(network.add_edge(XbeeRouting::Address(4), XbeeRouting::Address(5)));
  ASSERT_TRUE(network.add_edge(XbeeRouting::Address(1), XbeeRouting::Address(154)));
  ASSERT_TRUE(network.add_edge(XbeeRouting::Address(18), XbeeRouting::Address(56)));

  ASSERT_TRUE(network.adjacent(XbeeRouting::Address(4), XbeeRouting::Address(5)));
  ASSERT_TRUE(network.adjacent(XbeeRouting::Address(1), XbeeRouting::Address(154)));
  ASSERT_TRUE(network.adjacent(XbeeRouting::Address(18), XbeeRouting::Address(56)));

  ASSERT_FALSE(network.add_node(XbeeRouting::Address(4)));
  ASSERT_FALSE(network.add_node(XbeeRouting::Address(5)));
  ASSERT_FALSE(network.add_node(XbeeRouting::Address(1)));
  ASSERT_FALSE(network.add_node(XbeeRouting::Address(154)));
  ASSERT_FALSE(network.add_node(XbeeRouting::Address(18)));
  ASSERT_FALSE(network.add_node(XbeeRouting::Address(56)));

  ASSERT_NE(network.edge(XbeeRouting::Address(4), XbeeRouting::Address(5)), nullptr);
  ASSERT_NE(network.edge(XbeeRouting::Address(1), XbeeRouting::Address(154)), nullptr);
  ASSERT_NE(network.edge(XbeeRouting::Address(18), XbeeRouting::Address(56)), nullptr);

  ASSERT_FALSE(network.add_edge(XbeeRouting::Address(4), XbeeRouting::Address(5)));
  ASSERT_FALSE(network.add_edge(XbeeRouting::Address(1), XbeeRouting::Address(154)));
  ASSERT_FALSE(network.add_edge(XbeeRouting::Address(18), XbeeRouting::Address(56)));

  ASSERT_EQ(network.self()->address, XbeeRouting::Address(1));
}

/**
 * Test add edge self loop
 */
TEST(NetworkTest, add_edgeSelfLoop) {
  const XbeeRouting::Address self(125);
  XbeeRouting::Network network(self);

  EXPECT_FALSE(network.add_edge(self, self));
  EXPECT_FALSE(network.add_edge(XbeeRouting::Address(43), XbeeRouting::Address(43)));
}

/**
 * Drop edge
 */
TEST(NetworkTest, dropEdge) {
  XbeeRouting::Network network(1);

  ASSERT_TRUE(network.add_edge(1, 2));
  ASSERT_TRUE(network.add_edge(5, 10));

  ASSERT_TRUE(network.adjacent(1, 2));
  ASSERT_TRUE(network.drop(1, 2));
  ASSERT_TRUE(network.adjacent(5, 10));
  ASSERT_TRUE(network.drop(5, 10));

  ASSERT_TRUE(network.add_edge(1, 2));
  ASSERT_TRUE(network.add_edge(5, 10));
}

//! Edges contins matcher
MATCHER_P2(EdgeArrayContains, exp, size, "") {
  for (int i = 0; i < size; i++)
    if (arg[0] == exp[i][0] && arg[1] == exp[i][1])
      return true;

  return false;
}

//! Edges array contains matcher
MATCHER_P2(EdgesUnorderedEq, exp, size, "") {
  for (int i = 0; i < size; i++)
    EXPECT_THAT(arg[i], EdgeArrayContains(exp, size)) \
        << "arg[" << i << "]= {" << (int)arg[i][0] << ", " << (int)arg[i][1] << "}\n";

  return true;
}

/**
 * Merge and graph test (the same graphs)
 */
TEST(NetworkTest, mergeTheSameGraphs) {
  XbeeRouting::Network network1(1);
  XbeeRouting::Network network2(2);

  XbeeRouting::Edge expectedEdges[4] = {
    {XbeeRouting::Address(1), XbeeRouting::Address(2)},
    {XbeeRouting::Address(1), XbeeRouting::Address(3)},
    {XbeeRouting::Address(3), XbeeRouting::Address(4)},
    {XbeeRouting::Address(3), XbeeRouting::Address(6)}
  };

  ASSERT_TRUE(network1.add_edge(XbeeRouting::Address(1), XbeeRouting::Address(3)));
  ASSERT_TRUE(network1.add_edge(XbeeRouting::Address(4), XbeeRouting::Address(3)));
  ASSERT_TRUE(network1.add_edge(XbeeRouting::Address(6), XbeeRouting::Address(3)));
  ASSERT_TRUE(network1.add_edge(XbeeRouting::Address(2), XbeeRouting::Address(1)));

  ASSERT_TRUE(network2.add_edge(XbeeRouting::Address(1), XbeeRouting::Address(3)));
  ASSERT_TRUE(network2.add_edge(XbeeRouting::Address(4), XbeeRouting::Address(3)));
  ASSERT_TRUE(network2.add_edge(XbeeRouting::Address(6), XbeeRouting::Address(3)));
  ASSERT_TRUE(network2.add_edge(XbeeRouting::Address(2), XbeeRouting::Address(1)));

  uint8_t edgesSize = 10;
  XbeeRouting::Edge* edges1 = network1.graph(edgesSize);

  ASSERT_EQ(edgesSize, 4);
  ASSERT_NE(edges1, nullptr);

  EXPECT_THAT(edges1, EdgesUnorderedEq(expectedEdges, 4));

  network2.merge(edges1, edgesSize);

  EXPECT_TRUE(network1.adjacent(XbeeRouting::Address(1), XbeeRouting::Address(3)));
  EXPECT_TRUE(network1.adjacent(XbeeRouting::Address(4), XbeeRouting::Address(3)));
  EXPECT_TRUE(network1.adjacent(XbeeRouting::Address(6), XbeeRouting::Address(3)));
  EXPECT_TRUE(network1.adjacent(XbeeRouting::Address(2), XbeeRouting::Address(1)));

  EXPECT_TRUE(network2.adjacent(XbeeRouting::Address(1), XbeeRouting::Address(3)));
  EXPECT_TRUE(network2.adjacent(XbeeRouting::Address(4), XbeeRouting::Address(3)));
  EXPECT_TRUE(network2.adjacent(XbeeRouting::Address(6), XbeeRouting::Address(3)));
  EXPECT_TRUE(network2.adjacent(XbeeRouting::Address(2), XbeeRouting::Address(1)));

  edgesSize = 10;
  XbeeRouting::Edge* edges2 = network2.graph(edgesSize);

  ASSERT_EQ(edgesSize, 4);
  ASSERT_NE(edges2, nullptr);

  EXPECT_THAT(edges2, EdgesUnorderedEq(expectedEdges, 4));
}

/**
 * Merg and graph test (not the same graphs)
 */
TEST(NetworkTest, mergeGraphs) {
  XbeeRouting::Network network1(1);
  XbeeRouting::Network network2(2);

  XbeeRouting::Edge expectedEdges1[4] = {
    {XbeeRouting::Address(1), XbeeRouting::Address(2)},
    {XbeeRouting::Address(1), XbeeRouting::Address(3)},
    {XbeeRouting::Address(3), XbeeRouting::Address(4)},
    {XbeeRouting::Address(3), XbeeRouting::Address(6)}
  };
  XbeeRouting::Edge expectedEdgesAfterMerge[6] = {
    {XbeeRouting::Address(1), XbeeRouting::Address(2)},
    {XbeeRouting::Address(1), XbeeRouting::Address(3)},
    {XbeeRouting::Address(1), XbeeRouting::Address(7)},
    {XbeeRouting::Address(3), XbeeRouting::Address(4)},
    {XbeeRouting::Address(3), XbeeRouting::Address(6)},
    {XbeeRouting::Address(7), XbeeRouting::Address(8)}
  };

  ASSERT_TRUE(network1.add_edge(XbeeRouting::Address(1), XbeeRouting::Address(3)));
  ASSERT_TRUE(network1.add_edge(XbeeRouting::Address(4), XbeeRouting::Address(3)));
  ASSERT_TRUE(network1.add_edge(XbeeRouting::Address(6), XbeeRouting::Address(3)));
  ASSERT_TRUE(network1.add_edge(XbeeRouting::Address(2), XbeeRouting::Address(1)));

  ASSERT_TRUE(network2.add_edge(XbeeRouting::Address(1), XbeeRouting::Address(3)));
  ASSERT_TRUE(network2.add_edge(XbeeRouting::Address(4), XbeeRouting::Address(3)));
  ASSERT_TRUE(network2.add_edge(XbeeRouting::Address(7), XbeeRouting::Address(8)));
  ASSERT_TRUE(network2.add_edge(XbeeRouting::Address(1), XbeeRouting::Address(7)));

  uint8_t edgesSize1 = 10;
  XbeeRouting::Edge* edges1 = network1.graph(edgesSize1);

  ASSERT_EQ(edgesSize1, 4);
  ASSERT_NE(edges1, nullptr);

  EXPECT_THAT(edges1, EdgesUnorderedEq(expectedEdges1, 4));

  network2.merge(edges1, edgesSize1);

  EXPECT_TRUE(network1.adjacent(XbeeRouting::Address(1), XbeeRouting::Address(3)));
  EXPECT_TRUE(network1.adjacent(XbeeRouting::Address(4), XbeeRouting::Address(3)));
  EXPECT_TRUE(network1.adjacent(XbeeRouting::Address(6), XbeeRouting::Address(3)));
  EXPECT_TRUE(network1.adjacent(XbeeRouting::Address(2), XbeeRouting::Address(1)));

  EXPECT_TRUE(network2.adjacent(XbeeRouting::Address(1), XbeeRouting::Address(3)));
  EXPECT_TRUE(network2.adjacent(XbeeRouting::Address(4), XbeeRouting::Address(3)));
  EXPECT_TRUE(network2.adjacent(XbeeRouting::Address(6), XbeeRouting::Address(3)));
  EXPECT_TRUE(network2.adjacent(XbeeRouting::Address(2), XbeeRouting::Address(1)));
  EXPECT_TRUE(network2.adjacent(XbeeRouting::Address(7), XbeeRouting::Address(8)));
  EXPECT_TRUE(network2.adjacent(XbeeRouting::Address(1), XbeeRouting::Address(7)));

  uint8_t edgesSize2 = 10;
  XbeeRouting::Edge* edges2 = network2.graph(edgesSize2);

  ASSERT_EQ(edgesSize2, 6);
  ASSERT_NE(edges2, nullptr);

  EXPECT_THAT(edges2, EdgesUnorderedEq(expectedEdgesAfterMerge, 6));
}

/**
 * Path test - single chain
 */
TEST(NetworkTest, pathSingleChain) {
  const XbeeRouting::Address self(1);
  XbeeRouting::Network network(self);

  SET_EDGE(network, 1, 2, 10, 8, 10);
  SET_EDGE(network, 2, 3, 1, 4, 10);
  SET_EDGE(network, 4, 5, 15, 2, 2);
  SET_EDGE(network, 3, 4, 999, 0, 2);

  network.node(1)->mac = 1;
  network.node(2)->mac = 1;
  network.node(3)->mac = 1;
  network.node(4)->mac = 1;
  network.node(5)->mac = 1;

  XbeeRouting::Path path, expectedPath;

  expectedPath = { XbeeRouting::Address(2), XbeeRouting::Address(3), XbeeRouting::Address(4), XbeeRouting::Address(5) };
  path = network.path(XbeeRouting::Address(1), XbeeRouting::Address(5), XbeeRouting::Path());
  EXPECT_THAT(path, testing::ContainerEq(expectedPath));

  expectedPath = { XbeeRouting::Address(2), XbeeRouting::Address(3) };
  path = network.path(XbeeRouting::Address(1), XbeeRouting::Address(3), XbeeRouting::Path());
  EXPECT_THAT(path, testing::ContainerEq(expectedPath));

  expectedPath = { XbeeRouting::Address(2) };
  path = network.path(XbeeRouting::Address(1), XbeeRouting::Address(2), XbeeRouting::Path());
  EXPECT_THAT(path, testing::ContainerEq(expectedPath));
}

/**
 * Path test - simple no chain graph
 */
TEST(NetworkTest, pathNoChainGraph) {
  const XbeeRouting::Address self(1);
  XbeeRouting::Network network(self);

  SET_EDGE(network, 1, 2, 25, 1, 3);
  SET_EDGE(network, 2, 3, 85, 0, 7);
  SET_EDGE(network, 2, 5, 25, 1, 3);
  SET_EDGE(network, 4, 5, 5, 0, 3);
  SET_EDGE(network, 3, 4, 99, 0, 3);
  SET_EDGE(network, 6, 7, 50, 1, 8);
  SET_EDGE(network, 9, 8, 20, 2, 7);
  SET_EDGE(network, 8, 2, 15, 3, 3);
  SET_EDGE(network, 4, 1, 25, 1, 7);
  SET_EDGE(network, 4, 9, 15, 4, 13);
  SET_EDGE(network, 7, 8, 25, 1, 3);
  SET_EDGE(network, 3, 6, 10, 12, 43);

  network.node(1)->mac = 1;
  network.node(2)->mac = 1;
  network.node(3)->mac = 1;
  network.node(4)->mac = 1;
  network.node(5)->mac = 1;
  network.node(6)->mac = 1;
  network.node(7)->mac = 1;
  network.node(8)->mac = 1;
  network.node(9)->mac = 1;

  XbeeRouting::Path path, expectedPath;

  expectedPath = { XbeeRouting::Address(2), XbeeRouting::Address(8), XbeeRouting::Address(7), XbeeRouting::Address(6) };
  path = network.path(XbeeRouting::Address(1), XbeeRouting::Address(6), XbeeRouting::Path());
  EXPECT_THAT(path, testing::ContainerEq(expectedPath));
}

/**
 * Path test - simple tree graph
 */
TEST(NetworkTest, pathInTreeGraph) {
  const XbeeRouting::Address self(1);
  XbeeRouting::Network network(self);

  SET_EDGE(network, 1, 2, 81, 2, 7);
  SET_EDGE(network, 1, 8, 79, 8, 45);
  SET_EDGE(network, 1, 3, 57, 1, 2);
  SET_EDGE(network, 2, 4, 171, 3, 4);
  SET_EDGE(network, 2, 6, 111, 11, 32);
  SET_EDGE(network, 8, 6, 51, 14, 5);
  SET_EDGE(network, 8, 7, 65, 4, 6);
  SET_EDGE(network, 3, 7, 15, 3, 3);
  SET_EDGE(network, 3, 5, 171, 2, 3);
  SET_EDGE(network, 6, 9, 71, 21, 15);
  SET_EDGE(network, 7, 9, 71, 12, 38);

  network.node(1)->mac = 1;
  network.node(2)->mac = 1;
  network.node(3)->mac = 1;
  network.node(4)->mac = 1;
  network.node(5)->mac = 1;
  network.node(6)->mac = 1;
  network.node(7)->mac = 1;
  network.node(8)->mac = 1;
  network.node(9)->mac = 1;

  XbeeRouting::Path path, expectedPath;

  expectedPath = { XbeeRouting::Address(3), XbeeRouting::Address(7), XbeeRouting::Address(8), XbeeRouting::Address(6), XbeeRouting::Address(9) };
  path = network.path(XbeeRouting::Address(1), XbeeRouting::Address(9), XbeeRouting::Path());
  EXPECT_THAT(path, testing::ContainerEq(expectedPath));
}

/**
 * Path test - no path exists
 */
TEST(NetworkTest, pathNoPath) {
  const XbeeRouting::Address self(1);
  XbeeRouting::Network network(self);

  SET_EDGE(network, 1, 2, 92, 7, 11);
  SET_EDGE(network, 2, 3, 72, 4, 8);
  SET_EDGE(network, 2, 5, 82, 5, 21);
  SET_EDGE(network, 4, 5, 32, 1, 1);
  SET_EDGE(network, 3, 4, 45, 3, 41);
  SET_EDGE(network, 6, 7, 92, 4, 23);
  SET_EDGE(network, 9, 8, 54, 3, 51);
  SET_EDGE(network, 8, 2, 2, 2, 11);
  SET_EDGE(network, 4, 1, 72, 4, 41);
  SET_EDGE(network, 4, 9, 82, 2, 31);

  network.node(1)->mac = 1;
  network.node(2)->mac = 1;
  network.node(3)->mac = 1;
  network.node(4)->mac = 1;
  network.node(5)->mac = 1;
  network.node(6)->mac = 1;
  network.node(7)->mac = 1;
  network.node(8)->mac = 1;
  network.node(9)->mac = 1;

  XbeeRouting::Path path, expectedPath;

  expectedPath = { };
  path = network.path(XbeeRouting::Address(1), XbeeRouting::Address(6), XbeeRouting::Path());
  EXPECT_THAT(path, testing::ContainerEq(expectedPath));
}

/**
 * Path test - simple no chain graph (use visited)
 */
TEST(NetworkTest, pathNoChainGraphVisited) {
  const XbeeRouting::Address self(2);
  XbeeRouting::Network network(self);

  SET_EDGE(network, 1, 2, 25, 0, 3);
  SET_EDGE(network, 2, 3, 85, 1, 7);
  SET_EDGE(network, 2, 5, 25, 1, 3);
  SET_EDGE(network, 4, 5, 5, 0, 3);
  SET_EDGE(network, 3, 4, 99, 1, 3);
  SET_EDGE(network, 6, 7, 50, 1, 8);
  SET_EDGE(network, 9, 8, 20, 2, 7);
  SET_EDGE(network, 8, 2, 15, 3, 13);
  SET_EDGE(network, 4, 1, 95, 0, 0);
  SET_EDGE(network, 4, 9, 15, 4, 1);
  SET_EDGE(network, 7, 8, 25, 1, 3);
  SET_EDGE(network, 3, 6, 10, 12, 43);

  network.node(1)->mac = 1;
  network.node(2)->mac = 1;
  network.node(3)->mac = 1;
  network.node(4)->mac = 1;
  network.node(5)->mac = 1;
  network.node(6)->mac = 1;
  network.node(7)->mac = 1;
  network.node(8)->mac = 1;
  network.node(9)->mac = 1;

  XbeeRouting::Path path, expectedPath, visited;

  visited = { XbeeRouting::Address(1) };
  expectedPath = { XbeeRouting::Address(3), XbeeRouting::Address(4), XbeeRouting::Address(9), XbeeRouting::Address(8), XbeeRouting::Address(7), XbeeRouting::Address(6) };
  path = network.path(XbeeRouting::Address(2), XbeeRouting::Address(6), visited);
  EXPECT_THAT(path, testing::ContainerEq(expectedPath));
}

