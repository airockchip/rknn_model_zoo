/*
 * Copyright (C) 2016 Rockchip Electronics Co., Ltd.
 * Authors:
 *    Zhiqin Wei <wzq@rock-chips.com>
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
 
#ifndef _LIBS_RGA_SINGLETON_H
#define _LIBS_RGA_SINGLETON_H

#ifndef ANDROID
#include "RgaMutex.h"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundefined-var-template"
#endif

template <typename TYPE>
class Singleton {
  public:
    static TYPE& getInstance() {
        Mutex::Autolock _l(sLock);
        TYPE* instance = sInstance;
        if (instance == nullptr) {
            instance = new TYPE();
            sInstance = instance;
        }
        return *instance;
    }

    static bool hasInstance() {
        Mutex::Autolock _l(sLock);
        return sInstance != nullptr;
    }

  protected:
    ~Singleton() { }
    Singleton() { }

  private:
    Singleton(const Singleton&);
    Singleton& operator = (const Singleton&);
    static Mutex sLock;
    static TYPE* sInstance;
};

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#define RGA_SINGLETON_STATIC_INSTANCE(TYPE)                 \
    template<> ::Mutex  \
        (::Singleton< TYPE >::sLock)(::Mutex::PRIVATE);  \
    template<> TYPE* ::Singleton< TYPE >::sInstance(nullptr);  /* NOLINT */ \
    template class ::Singleton< TYPE >;

#endif //ANDROID
#endif //_LIBS_RGA_SINGLETON_H
