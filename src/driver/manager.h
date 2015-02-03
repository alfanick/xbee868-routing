#ifndef PUT_REDIS_MANAGER_H
#define PUT_REDIS_MANAGER_H

#include "../redis.h"
#include "subscription.h"

#include <hiredis/hiredis.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>

namespace PUT {
  namespace CS {
    namespace XbeeRouting {
      /**
       * Creates subscribers and manages a set of them.
       *
       * Created subscribers share the same prefix.
       */
      class Manager {
       private:
        //! Created subscribers
        std::unordered_map< std::string, Subscription> subscriptions;

        //! Key prefix
        char prefix[16];

        //! Redis connection (used for sending messages)
        redisContext* redis;

        //! Connection lock
        std::mutex redisLock;

       public:
        /**
         * Create Manager and connect to Redis.
         *
         * @param prefix Key prefix
         */
        Manager(std::string prefix);

        /**
         * Stop listeners and close Redis connection
         */
        ~Manager();

        /**
         * Subscribe to given channel and wait for given messages using
         * handler to process messages.
         *
         * @see PUT::XbeeRouting::Subscription
         * @param channel Channel name
         * @param handler Message handler
         */
        void subscribe(std::string channel, Handler handler);

        void subscribe(std::string channel, RawHandler handler);

        /**
         * Stop subscription channel.
         *
         * @param channel Channel name
         */
        void unsubscribe(std::string channel);

        /**
         * Stop listening every channel.
         */
        void unsubscribe();

        /**
         * Publish single message to given channel.
         *
         * @param channel Channel name (prefix:channel)
         * @param data Data
         * @return Number of subscribers who received message
         */
        int publish(const char* channel, std::string data);

        /**
         * Publish multiple messages to given channel.
         *
         * @param channel Channel name (prefix:channel)
         * @param data Vector of data (order is maintained)
         * @return Minimum number of subscribers who received any message
         */
        int publish(const char* channel, std::vector<std::string> data);

        int publish(const char* channel, uint8_t* data, size_t length);
        int publish(const char* channel, uint8_t* data, size_t length, const char* prefix);
      };
    }
  }
}
#endif
