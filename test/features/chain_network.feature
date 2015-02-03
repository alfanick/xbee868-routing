Feature: Testing basic chain network
  Background: 7 nodes network in perfect environment
    Given simulation of 02_chain network in perfect environment
    And every router is alive
    And topology is discovered

  Scenario: Send from first node to second node
    When 1 sends to 2 message "hello"

    Then 2 receives from 1 message "hello"

  Scenario: Send from first node to last node
    When 1 sends to 7 message "hello"

    Then 7 receives from 1 message "hello"
    And 1 receives acknowledge from 2
    And 2 receives acknowledge from 3
    And 3 receives acknowledge from 4
    And 4 receives acknowledge from 5
    And 5 receives acknowledge from 6
    And 6 receives acknowledge from 7

