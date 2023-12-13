/*
 * Copyright (C) 2020 Rockchip Electronics Co., Ltd.
 * Authors:
 *  PutinLee <putin.lee@rock-chips.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef _LIBS_RGA_MUTEX_H
#define _LIBS_RGA_MUTEX_H

#ifndef ANDROID
#include <stdint.h>
#include <sys/types.h>
#include <time.h>

#include <pthread.h>


// Enable thread safety attributes only with clang.
// The attributes can be safely erased when compiling with other compilers.
#if defined(__clang__) && (!defined(SWIG))
#define THREAD_ANNOTATION_ATTRIBUTE__(x) __attribute__((x))
#else
#define THREAD_ANNOTATION_ATTRIBUTE__(x)  // no-op
#endif

#define CAPABILITY(x) THREAD_ANNOTATION_ATTRIBUTE__(capability(x))

#define SCOPED_CAPABILITY THREAD_ANNOTATION_ATTRIBUTE__(scoped_lockable)

#define GUARDED_BY(x) THREAD_ANNOTATION_ATTRIBUTE__(guarded_by(x))

#define PT_GUARDED_BY(x) THREAD_ANNOTATION_ATTRIBUTE__(pt_guarded_by(x))

#define ACQUIRED_BEFORE(...) THREAD_ANNOTATION_ATTRIBUTE__(acquired_before(__VA_ARGS__))

#define ACQUIRED_AFTER(...) THREAD_ANNOTATION_ATTRIBUTE__(acquired_after(__VA_ARGS__))

#define REQUIRES(...) THREAD_ANNOTATION_ATTRIBUTE__(requires_capability(__VA_ARGS__))

#define REQUIRES_SHARED(...) THREAD_ANNOTATION_ATTRIBUTE__(requires_shared_capability(__VA_ARGS__))

#define ACQUIRE(...) THREAD_ANNOTATION_ATTRIBUTE__(acquire_capability(__VA_ARGS__))

#define ACQUIRE_SHARED(...) THREAD_ANNOTATION_ATTRIBUTE__(acquire_shared_capability(__VA_ARGS__))

#define RELEASE(...) THREAD_ANNOTATION_ATTRIBUTE__(release_capability(__VA_ARGS__))

#define RELEASE_SHARED(...) THREAD_ANNOTATION_ATTRIBUTE__(release_shared_capability(__VA_ARGS__))

#define TRY_ACQUIRE(...) THREAD_ANNOTATION_ATTRIBUTE__(try_acquire_capability(__VA_ARGS__))

#define TRY_ACQUIRE_SHARED(...) \
    THREAD_ANNOTATION_ATTRIBUTE__(try_acquire_shared_capability(__VA_ARGS__))

#define EXCLUDES(...) THREAD_ANNOTATION_ATTRIBUTE__(locks_excluded(__VA_ARGS__))

#define ASSERT_CAPABILITY(x) THREAD_ANNOTATION_ATTRIBUTE__(assert_capability(x))

#define ASSERT_SHARED_CAPABILITY(x) THREAD_ANNOTATION_ATTRIBUTE__(assert_shared_capability(x))

#define RETURN_CAPABILITY(x) THREAD_ANNOTATION_ATTRIBUTE__(lock_returned(x))

#define NO_THREAD_SAFETY_ANALYSIS THREAD_ANNOTATION_ATTRIBUTE__(no_thread_safety_analysis)

class Condition;

/*
 * NOTE: This class is for code that builds on Win32.  Its usage is
 * deprecated for code which doesn't build for Win32.  New code which
 * doesn't build for Win32 should use std::mutex and std::lock_guard instead.
 *
 * Simple mutex class.  The implementation is system-dependent.
 *
 * The mutex must be unlocked by the thread that locked it.  They are not
 * recursive, i.e. the same thread can't lock it multiple times.
 */
class CAPABILITY("mutex") Mutex {
  public:
    enum {
        PRIVATE = 0,
        SHARED = 1
    };

    Mutex();
    explicit Mutex(const char* name);
    explicit Mutex(int type, const char* name = nullptr);
    ~Mutex();

    // lock or unlock the mutex
    int32_t lock() ACQUIRE();
    void unlock() RELEASE();

    // lock if possible; returns 0 on success, error otherwise
    int32_t tryLock() TRY_ACQUIRE(0);

    int32_t timedLock(int64_t timeoutNs) TRY_ACQUIRE(0);

    // Manages the mutex automatically. It'll be locked when Autolock is
    // constructed and released when Autolock goes out of scope.
    class SCOPED_CAPABILITY Autolock {
      public:
        inline explicit Autolock(Mutex& mutex) ACQUIRE(mutex) : mLock(mutex) {
            mLock.lock();
        }
        inline explicit Autolock(Mutex* mutex) ACQUIRE(mutex) : mLock(*mutex) {
            mLock.lock();
        }
        inline ~Autolock() RELEASE() {
            mLock.unlock();
        }

      private:
        Mutex& mLock;
        // Cannot be copied or moved - declarations only
        Autolock(const Autolock&);
        Autolock& operator=(const Autolock&);
    };

  private:
    friend class Condition;

    // A mutex cannot be copied
    Mutex(const Mutex&);
    Mutex& operator=(const Mutex&);

    pthread_mutex_t mMutex;
};

// ---------------------------------------------------------------------------
inline Mutex::Mutex() {
    pthread_mutex_init(&mMutex, nullptr);
}
inline Mutex::Mutex(__attribute__((unused)) const char* name) {
    pthread_mutex_init(&mMutex, nullptr);
}
inline Mutex::Mutex(int type, __attribute__((unused)) const char* name) {
    if (type == SHARED) {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&mMutex, &attr);
        pthread_mutexattr_destroy(&attr);
    } else {
        pthread_mutex_init(&mMutex, nullptr);
    }
}
inline Mutex::~Mutex() {
    pthread_mutex_destroy(&mMutex);
}
inline int32_t Mutex::lock() {
    return -pthread_mutex_lock(&mMutex);
}
inline void Mutex::unlock() {
    pthread_mutex_unlock(&mMutex);
}
inline int32_t Mutex::tryLock() {
    return -pthread_mutex_trylock(&mMutex);
}
inline int32_t Mutex::timedLock(int64_t timeoutNs) {
    timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    timeoutNs += now.tv_sec*1000000000 + now.tv_nsec;
    const struct timespec ts = {
        /* .tv_sec = */ static_cast<time_t>(timeoutNs / 1000000000),
        /* .tv_nsec = */ static_cast<long>(timeoutNs % 1000000000),
    };
    return -pthread_mutex_timedlock(&mMutex, &ts);
}

// ---------------------------------------------------------------------------

/*
 * Automatic mutex.  Declare one of these at the top of a function.
 * When the function returns, it will go out of scope, and release the
 * mutex.
 */

typedef Mutex::Autolock AutoMutex;
#endif // __ANDROID_VNDK__
#endif // _LIBS_RGA_MUTEX_H
