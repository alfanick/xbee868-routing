#ifndef PUT_RADIO_DRIVER_H
#define PUT_RADIO_DRIVER_H

#include <functional>

#include "../radio.h"

#include "manager.h"

namespace PUT {
  namespace CS {
    namespace XbeeRouting {
      // destination port source data length
      typedef std::function<void (Address, uint8_t, Address, uint8_t*, size_t)> Action;

      // source data length
      typedef std::function<void (Address, uint8_t*, size_t)> LocalAction;

      class Driver {
       private:
        XbeeRouting::Manager* redis;
        XbeeRouting::Manager* undelivered_redis;

       public:
        const static Address SELF = 0;

        Driver();
        ~Driver();

        void self(Action action);
        void undelivered(Action action);

        void listen(uint8_t port, LocalAction action);
        void listen(Address address, uint8_t port, Action action);

        void deliver(Address destination, uint8_t port, uint8_t* data, size_t length);

        void deliver(Address source, Address destination, uint8_t port, uint8_t* data, size_t length);
        void deliver_back(Address destination, uint8_t port, uint8_t* data, size_t length);
      };
    }
  }
}
#endif
