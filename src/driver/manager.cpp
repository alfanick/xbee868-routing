#include "manager.h"

#include <glog/logging.h>
#include <cstring>

namespace PUT {
  namespace CS {
    namespace XbeeRouting {
      Manager::Manager(std::string p) {
        redisLock.lock();
        strcpy(prefix, p.c_str());
        redis = getConnection();
        redisLock.unlock();
      }

      Manager::~Manager() {
        DLOG(INFO) << "Removing all subscribers";

        for (auto &kv : subscriptions) {
          kv.second.stop();
        }

        subscriptions.clear();

        redisLock.lock();
        redisFree(redis);
      }

      int Manager::publish(const char* channel, uint8_t* data, size_t length, const char* prefix) {
        static char* db = getenv("REDIS_DATABASE");
        redisReply* reply;
        redisLock.lock();

        if (db == NULL) {
          reply = (redisReply*)redisCommand(redis, "PUBLISH %s:%s %b", prefix, channel, (char*)data, length);
        } else {
          reply = (redisReply*)redisCommand(redis, "PUBLISH %s/%s:%s %b", db, prefix, channel, (char*)data, length);
        }

        redisLock.unlock();
        int sub = reply->integer;

        freeReplyObject(reply);
        return sub;

      }

      int Manager::publish(const char* channel, uint8_t* data, size_t length) {
        return publish(channel, data, length, prefix);
      }

      int Manager::publish(const char* channel, std::string data) {
        return publish(channel, (uint8_t*)(data.c_str()), data.size());
      }

      int Manager::publish(const char* channel, std::vector<std::string> data) {
        int sub = INT32_MAX;

        for (const auto &d : data) {
          sub = std::min(sub, publish(channel, d));
        }

        return sub;
      }

      void Manager::subscribe(std::string channel, Handler handler) {
        subscribe(channel, (RawHandler)([ = ](uint8_t* data, size_t length, char* name) {
          handler(std::string((char*) data));
        }));
      }

      void Manager::subscribe(std::string channel, RawHandler handler) {
        //Subscription sub(prefix, channel, handler);
        subscriptions.emplace(std::piecewise_construct, std::forward_as_tuple(channel), std::forward_as_tuple(prefix, channel, handler));

        subscriptions[channel].run();
        DLOG(INFO) << "Subscribing to " << prefix << ":" << channel;
      }

      void Manager::unsubscribe(std::string channel) {
        DLOG(INFO) << "Unsubscribe from " << channel;
        subscriptions[channel].stop();
      }

      void Manager::unsubscribe() {
        DLOG(INFO) << "Unsubscribing all";

        for (auto &kv : subscriptions) {
          kv.second.stop();
        }
      }
    }
  }
}
