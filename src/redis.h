#ifndef PUT_REDIS_H
#define PUT_REDIS_H

#include "common.h"

#include <hiredis/hiredis.h>
#include <glog/logging.h>
#include <cstdlib>
#include <functional>
#include <string>
#include <cstring>
#include <iostream>

namespace PUT {
  namespace CS {
    namespace XbeeRouting {
      //! Redis message handler
      typedef std::function<void (std::string)> Handler;

      //! Raw message redis handler
      typedef std::function<void (uint8_t*, size_t, char*)> RawHandler;

      static inline redisContext* getConnection() {
        char* host;
        char* port;
        char* database;
        redisContext* context = nullptr;

        host = getenv("REDIS_HOST");
        port = getenv("REDIS_PORT");
        database = getenv("REDIS_DATABASE");

        if (host != nullptr) {
          LOG(INFO) << "REDIS_HOST specified, trying to connect to " << host << ":" << port ;
          context = redisConnect(host, port != nullptr ? atoi(port) : 6379);
        } else {
          LOG(INFO) << "REDIS_HOST unspecifed, trying to connect to /tmp/redis.sock";
          context = redisConnectUnix("/tmp/redis.sock");
        }

        if (context != nullptr && context->err) {
          LOG(FATAL) << "Could not connect to redis, make sure it's up and running";
        }

        if (database != nullptr) {
          LOG(INFO) << "Choosing " << database << " database";
          freeReplyObject(redisCommand(context, "SELECT %s", database));
        }

        return context;
      }
    }
  }
}
#endif
