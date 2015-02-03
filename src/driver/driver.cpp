#include "driver.h"

namespace PUT {
  namespace CS {
    namespace XbeeRouting {
      Driver::Driver() {
        redis = new XbeeRouting::Manager("network");
        undelivered_redis = new XbeeRouting::Manager("undelivered");
      }

      Driver::~Driver() {
        delete redis;
        delete undelivered_redis;
      }

      void Driver::listen(uint8_t p, LocalAction action) {
        listen(Driver::SELF, p, [ = ](Address destination, uint8_t port, Address source, uint8_t* data, size_t length) {
          action(source, data, length);
        });
      }

      void Driver::self(Action action) {
        char channel_name[] = "*:*:self";

        redis->subscribe(channel_name, (XbeeRouting::RawHandler)([ = ](uint8_t* data, size_t length, char* channel) {
          Address destination = 0;
          uint8_t port = 0;
          char* token_tmp;
          char* token = strtok_r(channel, ":", &token_tmp);
          int position = 0;

          while ((token = strtok_r(NULL, ":", &token_tmp)) != NULL) {
            if (position == 0) {
              port = atoi(token);
            } else if (position == 1) {
              destination = strtol(token, NULL, 10);
            }

            position++;
          }


          action(destination, port, Driver::SELF, data, length);
        }));
      }

      void Driver::undelivered(Action action) {
        char channel_name[] = "*:*:self";

        undelivered_redis->subscribe(channel_name, (XbeeRouting::RawHandler)([ = ](uint8_t* data, size_t length, char* channel) {
          Address destination = 0;
          uint8_t port = 0;
          char* token_tmp;
          char* token = strtok_r(channel, ":", &token_tmp);
          int position = 0;

          while ((token = strtok_r(NULL, ":", &token_tmp)) != NULL) {
            if (position == 0) {
              port = atoi(token);
            } else if (position == 1) {
              destination = strtol(token, NULL, 10);
            }

            position++;
          }


          action(destination, port, Driver::SELF, data, length);
        }));
      }

      void Driver::listen(Address destination, uint8_t port, Action action) {
        char channel_name[40];

        if (destination == Driver::SELF) {
          sprintf(channel_name, "%d:self:*", port);
        } else {
          sprintf(channel_name, "%d:%d:*", port, destination);
        }

        redis->subscribe(channel_name, (XbeeRouting::RawHandler)([ = ](uint8_t* data, size_t length, char* channel) {
          char* position = strrchr(channel, ':');
          Address source = ((unsigned)(position - channel + 1) >= (strlen(channel_name) + 7)) ? atoi(position + 1) : 0;

          action(destination, port, source, data, length);
        }));
      }

      void Driver::deliver(Address destination, uint8_t port, uint8_t* data, size_t length) {
        deliver(Driver::SELF, destination, port, data, length);
      }

      void Driver::deliver(Address source, Address destination, uint8_t port, uint8_t* data, size_t length) {
        char channel_name[40];

        if (destination == Driver::SELF) {
          if (source == Driver::SELF) {
            sprintf(channel_name, "%d:self:self", port);
          } else {
            sprintf(channel_name, "%d:self:%d", port, source);
          }
        } else {
          if (source == Driver::SELF) {
            sprintf(channel_name, "%d:%d:self", port, destination);
          } else {
            sprintf(channel_name, "%d:%d:%d", port, destination, source);
          }
        }

        redis->publish(channel_name, data, length);
      }

      void Driver::deliver_back(Address destination, uint8_t port, uint8_t* data, size_t length) {
        char channel_name[40];

        sprintf(channel_name, "%d:%d:self", port, destination);

        redis->publish(channel_name, data, length, "undelivered");
      }
    }
  }
}
