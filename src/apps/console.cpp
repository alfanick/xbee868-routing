#include "../driver/driver.h"
#include <cstdio>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>

using namespace std;
using namespace PUT::CS::XbeeRouting;

void parse_command(char* command, Driver* driver, uint8_t port);


/**
 * Simple console that allow us to spy packets on given port
 * and send data packets to given node on a given port
 *
 * @param port to listen
 * @param path to file or device to which information about packets will be written (without this parameter output will be sent to stdout)
 *
 * EXAMPLES of launching console:
 *  ./console 10 /dev/ttys001
 *  It causes that console listen to port 10 and output is sent to /dev/ttys001
 *
 *  ./console 10
 *  It causes that console listen to port 10 and output is sent to stdout
 *
 * SENDING PACKETS:
 *  structure of command: "action destination data"
 *
 *  possible actions:
 *      "deliver" (or "d") - sends packet to given node on given port using PUT::CS::XbeeRouting::Driver::deliver
 *
 *    example:
 *      deliver 2 some_kind_of_data - it will send some_kind_of_data to node 2 on port given as argument to console
 *
 */
int main(int argc, char* argv[]) {
  Driver* driver = new Driver();
  FILE* descriptor;
  bool run = true;

  uint8_t port;

  if (argc == 1) {
    port = 15;
  } else {
    port = atoi(argv[1]);
  }

  if (argc > 2) {
    descriptor = fopen(argv[2], "a");

    if (!descriptor) {
      printf("wrong path to the terminal typed, try again...\n");
      return 0;
    }
  } else {
    descriptor = fopen("/dev/stdout", "a");
  }


  driver->listen(Driver::SELF, port, [ = ](Address destination, uint8_t p, Address source, uint8_t* data, size_t length) {
    if (source == Driver::SELF)
      fprintf(descriptor, "packet send:\n\tto: %d, port: %d, length: %zu\n\tdata: %s\n\n", destination, p, length, data);
    else
      fprintf(descriptor, "packet received:\n\tfrom: %d, port: %d, length: %zu\n\tdata: %s\n\n", source, p, length, data);
  });

  char* input, shell_prompt[100];

  while (run) {
    // create prompt
    snprintf(shell_prompt, sizeof(shell_prompt), "port:%d> ", port);

    input = readline(shell_prompt);

    if (!input)
      break;

    if (strcmp(input, "") != 0)
      add_history(input);

    if (strcmp(input, "exit") == 0)
      run = false;
    else
      parse_command(input, driver, port);

    free(input);
  }

  return 0;
}


void parse_command(char* command, Driver* driver, uint8_t port) {
  char* action = (char*)malloc(sizeof(char) * 10);
  int destination = 0;
  char* data = (char*)malloc(sizeof(char) * 255);

  sscanf(command, "%s %d %[^\n]s", action, &destination, data);

  if ((strcmp(action, "deliver") == 0) || (strcmp(action, "d") == 0)) {
    driver->deliver(destination, port, (uint8_t*)data, strlen(data));
  }

  free(data);
  free(action);
}