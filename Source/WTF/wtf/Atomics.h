/*
 * Copyright (C) 2007, 2008, 2010, 2012, 2013 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Justin Haygood (jhaygood@reaktix.com)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * Note: The implementations of InterlockedIncrement and InterlockedDecrement are based
 * on atomic_increment and atomic_exchange_and_add from the Boost C++ Library. The license
 * is virtually identical to the Apple license above but is included here for completeness.
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 * 
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 * 
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef Atomics_h
#define Atomics_h

#include <wtf/Platform.h>
#include <wtf/StdLibExtras.h>

#if OS(WINDOWS)
#if OS(WINCE)
#include <cmnintrin.h>
#elif !COMPILER(GCC)
extern "C" void _ReadWriteBarrier(void);
#pragma intrinsic(_ReadWriteBarrier)
#endif
#include <windows.h>
#elif OS(QNX)
#include <atomic.h>
#elif OS(ANDROID)
#include <sys/atomics.h>
#endif
#include "iostream"
#include <signal.h>

namespace WTF {

// 32bit atomicIncrement/atomicDecrement
#if OS(WINDOWS)
#define WTF_USE_LOCKFREE_THREADSAFEREFCOUNTED 1

#if OS(WINCE) || COMPILER(MINGW)
inline int atomicIncrement(int* addend) { return InterlockedIncrement(reinterpret_cast<long*>(addend)); }
inline int atomicDecrement(int* addend) { return InterlockedDecrement(reinterpret_cast<long*>(addend)); }
#else
inline int atomicIncrement(int volatile* addend) { return InterlockedIncrement(reinterpret_cast<long volatile*>(addend)); }
inline int atomicDecrement(int volatile* addend) { return InterlockedDecrement(reinterpret_cast<long volatile*>(addend)); }
#endif

#elif OS(QNX)
#define WTF_USE_LOCKFREE_THREADSAFEREFCOUNTED 1

// Note, atomic_{add, sub}_value() return the previous value of addend's content.
inline int atomicIncrement(int volatile* addend) { return static_cast<int>(atomic_add_value(reinterpret_cast<unsigned volatile*>(addend), 1)) + 1; }
inline int atomicDecrement(int volatile* addend) { return static_cast<int>(atomic_sub_value(reinterpret_cast<unsigned volatile*>(addend), 1)) - 1; }

#elif OS(ANDROID)
#define WTF_USE_LOCKFREE_THREADSAFEREFCOUNTED 1

// Note, __atomic_{inc, dec}() return the previous value of addend's content.
inline int atomicIncrement(int volatile* addend) { return __atomic_inc(addend) + 1; }
inline int atomicDecrement(int volatile* addend) { return __atomic_dec(addend) - 1; }

#elif COMPILER(GCC)
#define WTF_USE_LOCKFREE_THREADSAFEREFCOUNTED 1

inline int atomicIncrement(int volatile* addend) { return __sync_add_and_fetch(addend, 1); }
inline int atomicDecrement(int volatile* addend) { return __sync_sub_and_fetch(addend, 1); }

#endif

// 64bit atomicIncrement/atomicDecrement
#if HAVE(ATOMICS_64BIT)
#if COMPILER(GCC)
inline int64_t atomicIncrement(int64_t volatile* addend) { return __sync_add_and_fetch(addend, 1); }
inline int64_t atomicDecrement(int64_t volatile* addend) { return __sync_sub_and_fetch(addend, 1); }
#elif OS(WINDOWS)
inline int64_t atomicIncrement(int64_t volatile* addend) { return InterlockedIncrement64(reinterpret_cast<long long volatile*>(addend)); }
inline int64_t atomicDecrement(int64_t volatile* addend) { return InterlockedDecrement64(reinterpret_cast<long long volatile*>(addend)); }
#endif
#endif


#if OS(WINDOWS)
inline bool weakCompareAndSwap(volatile unsigned* location, unsigned expected, unsigned newValue)
{
#if OS(WINCE)
    return InterlockedCompareExchange(reinterpret_cast<LONG*>(const_cast<unsigned*>(location)), static_cast<LONG>(newValue), static_cast<LONG>(expected)) == static_cast<LONG>(expected);
#else
    return InterlockedCompareExchange(reinterpret_cast<LONG volatile*>(location), static_cast<LONG>(newValue), static_cast<LONG>(expected)) == static_cast<LONG>(expected);
#endif
}

inline bool weakCompareAndSwap(void*volatile* location, void* expected, void* newValue)
{
    return InterlockedCompareExchangePointer(location, newValue, expected) == expected;
}
#else // OS(WINDOWS) --> not windows
#if COMPILER(GCC) && !COMPILER(CLANG) // Work around a gcc bug 
inline bool weakCompareAndSwap(volatile unsigned* location, unsigned expected, unsigned newValue) 
#else
inline bool weakCompareAndSwap(unsigned* location, unsigned expected, unsigned newValue)
#endif
{
#if ENABLE(COMPARE_AND_SWAP)
#if CPU(X86) || CPU(X86_64)
    unsigned char result;
    asm volatile(
        "lock; cmpxchgl %3, %2\n\t"
        "sete %1"
        : "+a"(expected), "=q"(result), "+m"(*location)
        : "r"(newValue)
        : "memory"
        );
#elif CPU(ARM_THUMB2)
    unsigned tmp;
    unsigned result;
    asm volatile(
        "movw %1, #1\n\t"
        "ldrex %2, %0\n\t"
        "cmp %3, %2\n\t"
        "bne.n 0f\n\t"
        "strex %1, %4, %0\n\t"
        "0:"
        : "+Q"(*location), "=&r"(result), "=&r"(tmp)
        : "r"(expected), "r"(newValue)
        : "memory");
    result = !result;
//#elif ((defined(__ppc64__) || defined(__PPC64__)) && defined (__LITTLE_ENDIAN__))  /* ATUL */
#elif (CPU(PPC64) && defined (__LITTLE_ENDIAN__))  /* ATUL */
#warning "ATUL>>> COMPARE_AND_SWAP enabled, using gcc built-in"
std::cerr << "ATUL>>> COMPARE_AND_SWAP enabled, nusing gcc built-in." << std::endl;
    return __sync_bool_compare_and_swap (location, expected, newValue);
#else
#error "Bad architecture for compare and swap."
#endif
#else    // COMPARE_AND_SWAP --> not enabled.
std::cerr << "ATUL>>> COMPARE_AND_SWAP disabled" << std::endl;
    UNUSED_PARAM(location);
    UNUSED_PARAM(expected);
    UNUSED_PARAM(newValue);
    CRASH();
    return false;
#endif
}

inline bool weakCompareAndSwap(void*volatile* location, void* expected, void* newValue)
{
#if ENABLE(COMPARE_AND_SWAP)
#if CPU(X86_64)
    bool result;
    asm volatile(
        "lock; cmpxchg %3, %2\n\t"
        "sete %1"
        : "+a"(expected), "=q"(result), "+m"(*location)
        : "r"(newValue)
        : "memory"
        );
    return result;
//#elif ((defined(__ppc64__) || defined(__PPC64__)) && defined (__LITTLE_ENDIAN__))  /* ATUL */
#elif (CPU(PPC64) && defined (__LITTLE_ENDIAN__))  /* ATUL */
#warning "ATUL>>> calling weakCompareAndSwap for ppc64le from here-using gcc builr-in."
std::cerr << "ATUL>>> calling weakCompareAndSwap for ppc64le from here-using gcc builr-in." << std::endl;
    return __sync_bool_compare_and_swap (location, expected, newValue);
#else
    return weakCompareAndSwap(bitwise_cast<unsigned*>(location), bitwise_cast<unsigned>(expected), bitwise_cast<unsigned>(newValue));
#endif
#else // ENABLE(COMPARE_AND_SWAP)
    UNUSED_PARAM(location);
    UNUSED_PARAM(expected);
    UNUSED_PARAM(newValue);
    CRASH();
    return 0;
#endif // ENABLE(COMPARE_AND_SWAP)
}
#endif // OS(WINDOWS) (end of the not-windows case)

inline bool weakCompareAndSwapUIntPtr(volatile uintptr_t* location, uintptr_t expected, uintptr_t newValue)
{
    return weakCompareAndSwap(reinterpret_cast<void*volatile*>(location), reinterpret_cast<void*>(expected), reinterpret_cast<void*>(newValue));
}

// Just a compiler fence. Has no effect on the hardware, but tells the compiler
// not to move things around this call. Should not affect the compiler's ability
// to do things like register allocation and code motion over pure operations.
inline void compilerFence()
{
#if OS(WINDOWS) && !COMPILER(GCC)
    _ReadWriteBarrier();
#else
    asm volatile("" ::: "memory");
#endif
}

#if CPU(ARM_THUMB2)

// Full memory fence. No accesses will float above this, and no accesses will sink
// below it.
inline void armV7_dmb()
{
    asm volatile("dmb" ::: "memory");
}

// Like the above, but only affects stores.
inline void armV7_dmb_st()
{
    asm volatile("dmb st" ::: "memory");
}

inline void loadLoadFence() { armV7_dmb(); }
inline void loadStoreFence() { armV7_dmb(); }
inline void storeLoadFence() { armV7_dmb(); }
inline void storeStoreFence() { armV7_dmb_st(); }
inline void memoryBarrierAfterLock() { armV7_dmb(); }
inline void memoryBarrierBeforeUnlock() { armV7_dmb(); }

/* ATUL */
//#elif ((defined(__ppc64__) || defined(__PPC64__)) && defined (__LITTLE_ENDIAN__))
#elif (CPU(PPC64) && defined (__LITTLE_ENDIAN__))
#warning "ATUL>>> Using ppc64le code for fencing"
inline void ppc64le_hwsync()
{
std::cerr << ">>> ATUL: in ppc64le equiv. compilerFence" << std::endl;
    asm volatile("sync\n\t" ::: "memory");
}

inline void loadLoadFence() { ppc64le_hwsync(); }
inline void loadStoreFence() { ppc64le_hwsync(); }
inline void storeLoadFence() { ppc64le_hwsync(); }
inline void storeStoreFence() { ppc64le_hwsync(); }
inline void memoryBarrierAfterLock() { ppc64le_hwsync(); }
inline void memoryBarrierBeforeUnlock() { ppc64le_hwsync(); }
/* ATUL */
#elif CPU(X86) || CPU(X86_64)

inline void x86_mfence()
{
#if OS(WINDOWS)
    // I think that this does the equivalent of a dummy interlocked instruction,
    // instead of using the 'mfence' instruction, at least according to MSDN. I
    // know that it is equivalent for our purposes, but it would be good to
    // investigate if that is actually better.
    MemoryBarrier();
#else
    asm volatile("mfence" ::: "memory");
#endif
}

inline void loadLoadFence() { compilerFence(); }
inline void loadStoreFence() { compilerFence(); }
inline void storeLoadFence() { x86_mfence(); }
inline void storeStoreFence() { compilerFence(); }
inline void memoryBarrierAfterLock() { compilerFence(); }
inline void memoryBarrierBeforeUnlock() { compilerFence(); }

#else
#warning "ATUL>>> Using generic code for fencing"

inline void loadLoadFence() { compilerFence(); }
inline void loadStoreFence() { compilerFence(); }
inline void storeLoadFence() { compilerFence(); }
inline void storeStoreFence() { compilerFence(); }
inline void memoryBarrierAfterLock() { compilerFence(); }
inline void memoryBarrierBeforeUnlock() { compilerFence(); }

#endif

} // namespace WTF

#if USE(LOCKFREE_THREADSAFEREFCOUNTED)
using WTF::atomicDecrement;
using WTF::atomicIncrement;
#endif

#endif // Atomics_h
