#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <stdexcept>
#include "../driver/driver.h"

using namespace std;
using namespace PUT::CS::XbeeRouting;

float temperature() {
  ifstream f("/sys/class/thermal/thermal_zone0/temp");

  if (!f.is_open())
    return -1;

  int temperature = 0;
  f >> temperature;

  return temperature / 1000.0;
}

int main(int argc, char const* argv[]) {
  Driver* driver = new Driver();
  uint8_t port;

  if (argc == 1) {
    port = 7;
  } else {
    port = atoi(argv[1]);
  }

  driver->listen(port, [ = ](Address source, uint8_t* data, size_t length) {
    if (source != Driver::SELF) {
      if (strncmp((char*)data, "gettemp", 7) == 0) {
        string s = to_string(temperature());
        s += "'C";

        driver->deliver(source, port, (uint8_t*)s.c_str(), s.length());
      }
    }
  });

  pause();
  return 0;
}
