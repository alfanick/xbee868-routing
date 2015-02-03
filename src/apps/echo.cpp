#include "../driver/driver.h"

using namespace PUT::CS::XbeeRouting;

int main(int argc, char const* argv[]) {
  Driver* driver = new Driver();

  uint8_t port;

  if (argc == 1) {
    port = 15;
  } else {
    port = atoi(argv[1]);
  }

  driver->listen(port, [ = ](Address source, uint8_t* data, size_t length) {
    if (source != Driver::SELF)
      driver->deliver(source, port, data, length);
  });

  pause();
  return 0;
}