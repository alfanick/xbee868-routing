#ifndef PUT_COMMON_H
#define PUT_COMMON_H

#include <pthread.h>

#if RASPBERRY || __linux
#define THREAD_NAME(x) pthread_setname_np(pthread_self(), (x))
#else
#define THREAD_NAME(x) pthread_setname_np((x))
#endif

#endif
