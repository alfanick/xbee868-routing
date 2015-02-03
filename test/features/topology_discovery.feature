Feature: Testing topology discovery
  Background: 4 nodes network
    Given simulation of 00_basic network in perfect environment

  Scenario: Testing broadcasting of node in raw form
    When router alfa is alive

    Then alfa broadcasts data FE SS

  Scenario: Testing broadcasting of node
    When router alfa is alive

    Then node is broadcasted from alfa

  Scenario: Testing broadcasting of graph
    When router alfa is alive
    And router beta is alive

    Then node is broadcasted from alfa
    And node is broadcasted from beta
    And graph is broadcasted from alfa

  Scenario: Testing topology discovery
    When every router is alive

    Then topology is discovered
