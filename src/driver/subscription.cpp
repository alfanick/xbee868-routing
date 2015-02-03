#include "subscription.h"
#include <string.h>
#include <cstdlib>

namespace PUT {
  namespace CS {
    namespace XbeeRouting {
      Subscription::Subscription(std::string prefix, std::string channel, RawHandler h) {
        char* db = getenv("REDIS_DATABASE");

        if (db != NULL) {
          prefix.insert(0, std::string(db) + "/");
        }

        strcpy(subscribe, ("PSUBSCRIBE " + prefix + ":" + channel).c_str());
        strcpy(unsubscribe, ("UNSUBSCRIBE " + prefix + ":" + channel).c_str());

        handler = h;

        redis = getConnection();
      }

      Subscription::Subscription(const Subscription &other) : handler(other.handler) {
        strcpy(subscribe, other.subscribe);
        strcpy(unsubscribe, other.unsubscribe);
        redis = getConnection();
      }

      Subscription::Subscription() {

      }

      Subscription::~Subscription() {
        stop();

        redisFree(redis);
      }

      void Subscription::run() {
        should_run.store(true);

        thread = std::thread([ = ]() {
          THREAD_NAME("Redis subscription");

          void* reply;

          freeReplyObject(redisCommand(redis, subscribe));

          while (should_run.load() && (redisGetReply(redis, &reply) == REDIS_OK)) {
            if (((redisReply*)reply)->type == REDIS_REPLY_ARRAY) {
              if (strcmp(((redisReply*)reply)->element[0]->str, "pmessage") == 0) {
                handler((uint8_t*)(((redisReply*)reply)->element[3]->str), ((redisReply*)reply)->element[3]->len, ((redisReply*)reply)->element[2]->str);
              } else if (strcmp(((redisReply*)reply)->element[0]->str, "unsubscribe") == 0) {
                should_run.store(false);
              }
            }

            freeReplyObject(reply);
          }
        });
      }

      void Subscription::stop() {
        should_run.store(false);
        redisAppendCommand(redis, unsubscribe);

        if (thread.joinable())
          thread.join();
      }
    }
  }
}
