Feature: Testing basic network
  Background: 4 nodes network in perfect environment
    Given simulation of 00_basic network in perfect environment
    And every router is alive
    And topology is discovered

  Scenario: Send from first node to second node
    When alfa sends to beta message "hello"

    Then beta receives from alfa message "hello"

  Scenario: Send from first node to second node (test low level)
    When alfa sends to beta message "hello"

    Then alfa transmits to beta data 01 TT SS .. .. 00 68 65 6C 6C 6F
    And beta receives from alfa message "hello"

  Scenario: Send from first node to last node
    When alfa sends to delta message "hello"

    Then delta receives from alfa message "hello"
    And "hello" route is alfa, [beta, gamma], delta

  Scenario: Send from first node to last node, intermediate node is off
    Given node beta is down

    When alfa sends to delta message "hello"

    Then delta receives from alfa message "hello"
    And "hello" route is alfa, gamma, delta

  Scenario: Send from first node to last node, both intermediate nodes are off
    Given node beta is down
    And node gamma is down
    And receive timeout is 1s

    When alfa sends to delta message "hello"

    Then delta does not receive from alfa message "hello"
    And alfa receives back message "hello"

