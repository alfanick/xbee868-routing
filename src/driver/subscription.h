#ifndef PUT_REDIS_SUBSCRIPTION_H
#define PUT_REDIS_SUBSCRIPTION_H

#include "../redis.h"
#include <hiredis/hiredis.h>

#include <thread>
#include <atomic>

namespace PUT {
  namespace CS {
    namespace XbeeRouting {
      /**
       * Redis PUB/SUB implementation. Subscribe to single channel.
       */
      class Subscription {
       private:
        //! Subscribe command
        char subscribe[64];
        //! Unsubscribe command
        char unsubscribe[64];

        //! Thread status
        std::atomic_bool should_run;
        //! Thread
        std::thread thread;

        //! Message handler
        RawHandler handler;

        //! Redis connection
        redisContext* redis;
       public:
        /**
         * Create Subscription instance. Connect to Redis server
         * and use prefix:channel channel.
         *
         * @param prefix Prefix (like ui, settings, etc..)
         * @param channel Channel name
         * @param handler Message handler
         */
        Subscription(std::string prefix, std::string channel, RawHandler handler);
        Subscription(const Subscription &other);
        Subscription();

        /**
         * Stop subscription and close Redis connection
         */
        ~Subscription();

        /**
         * Create Subscription thread and route messages to handler.
         */
        void run();

        /**
         * Stop thread and unsubscribe
         */
        void stop();
      };
    }
  }
}
#endif
