// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#ifndef JOSUTILS_JTHREADING_HPP_HEADER_GUARD
#define JOSUTILS_JTHREADING_HPP_HEADER_GUARD

#include "jbase/jfatal.hpp"

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/time.h>
    #include <unistd.h>
    #include <pthread.h>
#endif

#include <cassert>
#include <cstdlib>

#include <algorithm>
#include <functional>
#include <limits>
#include <memory>
#include <vector>


namespace jjm
{


// **** **** 
//Forward Declarations
class Mutex;
class CondVar;
class Lock;
class AdvLock;
class ReverseLock;
class Thread;
void wait_I_understand_that_RAII_and_LockClass_are_better(Mutex& , CondVar& );


// **** **** 
// Public API


void sleep(unsigned long millisec);


//Mutex is a simple POSIX-like non-recursive mutex. 
class Mutex
{
public:
    Mutex();
    ~Mutex();
    void lock_I_understand_that_RAII_and_LockClass_are_better();
    void unlock_I_understand_that_RAII_and_LockClass_are_better();
    friend void wait_I_understand_that_RAII_and_LockClass_are_better(Mutex& , CondVar& );
private:
    Mutex(Mutex const& ); //not defined, not copyable
    Mutex& operator= (Mutex const& ); //not defined, not copyable

#ifdef _WIN32
    CRITICAL_SECTION criticalSection;
#else
    pthread_mutex_t mutex;
#endif
};


//CondVar is a simple POSIX-like condition variable. 
class CondVar
{
public:
    CondVar(); 
    ~CondVar();

    //The calling thread must own the mutex lock.
    //This will atomically release the mutex lock and wait on the condition. 
    //This has spurious wakeups. That is, it may return without a
    //corresponding notify_one or notify_all. (However, notify_one and
    //notify_all signals will not be "lost" with proper synchronization.) 
    friend void wait_I_understand_that_RAII_and_LockClass_are_better(Mutex& , CondVar& );

    void notify_one();

    void notify_all();

private:
    CondVar(CondVar const& ); //not defined, not copyable
    CondVar& operator= (CondVar const& ); //not defined, not copyable

#ifdef _WIN32
    CONDITION_VARIABLE conditionVariable; 
#else
    pthread_cond_t pthreadCond;
#endif
};


//Simple RAII class to obtain and release a Mutex
class Lock
{
public:
    Lock(Mutex & m_) : m(m_) { m.lock_I_understand_that_RAII_and_LockClass_are_better(); }
    ~Lock() { m.unlock_I_understand_that_RAII_and_LockClass_are_better(); }

    //This has spurious wakeups. That is, it may return without a 
    //corresponding notify_one() or notify_all(). 
    //As normal POSIX mutexes and condition variables, with proper use, 
    //notification from notify_one()  and notify_all() will not be "lost". 
    friend void wait(Lock& lock, CondVar& c) { wait_I_understand_that_RAII_and_LockClass_are_better(lock.m, c); } 

private:
    Lock(Lock const& ); //not defined, not copyable
    Lock& operator= (Lock const& ); //not defined, not copyable
    Mutex & m;

    friend class ReverseLock;
};


//Simple RAII class to release and obtain a Mutex
class ReverseLock
{
public:
    ReverseLock(Mutex& m) : m(m) { m.unlock_I_understand_that_RAII_and_LockClass_are_better(); }
    ReverseLock(Lock& g) : m(g.m) { m.unlock_I_understand_that_RAII_and_LockClass_are_better(); }
    ~ReverseLock() { m.lock_I_understand_that_RAII_and_LockClass_are_better(); }
private:
    ReverseLock(ReverseLock const& ); //not defined, not copyable
    ReverseLock& operator= (ReverseLock const& ); //not defined, not copyable
    Mutex & m;
};


//This will always lock the given Mutexes in a global order: 
//std::less when applied to the address of the Mutexes.
class MultiLock
{
public:
    MultiLock() : isLocked(false) {}
    ~MultiLock()
    {   if (isLocked)
            for (std::vector<T>::iterator m = mutexes.begin(); m != mutexes.end(); ++m)
                m->m->unlock_I_understand_that_RAII_and_LockClass_are_better();
    }

    void add(Mutex& m)
    {   if (isLocked) 
            JFATAL(0, 0);
        sortedVectorInsert(mutexes, &m);
    }

    //it will acquire all of the given mutexes in address order,
    void lock()
    {   for (std::vector<T>::iterator m = mutexes.begin(); m != mutexes.end(); ++m)
            m->m->lock_I_understand_that_RAII_and_LockClass_are_better();
        isLocked = true;
    }

    //For every mutex added which was already locked,
    //it is not unlocked here (or in the destructor).
    //All (other) mutexes added with add(...) are unlocked here (and in the destructor).
    void unlockAndClear()
    {   if (isLocked)
            for (std::vector<T>::iterator m = mutexes.begin(); m != mutexes.end(); ++m)
                m->m->unlock_I_understand_that_RAII_and_LockClass_are_better();
        isLocked = false;
        mutexes.clear();
    }

private:
    MultiLock(MultiLock const& ); //not defined, not copyable
    MultiLock& operator= (MultiLock const& ); //not defined, not copyable

    struct T
    {   Mutex* m; 
        bool operator< (T t) const { return std::less<Mutex*>()(m, t.m); }
    };
    std::vector<T> mutexes;
    static void sortedVectorInsert(std::vector<T>& v, Mutex* const m)
    {   T t;
        t.m = m;
        if (v.size() == 0)
        {   v.push_back(t);
            return;
        }
        T* const a = & v[0];
        T* const b = & v[0] + v.size();
        T* const x = std::lower_bound(a, b, t);
        if (x == b || x->m != m)
            v.insert(v.begin() + (x - & v[0]), t);
    }

    bool isLocked;
};


class DynamicLock
{
public:
    DynamicLock(Mutex * m_) : m(0) { setAndLock(m_); }
    ~DynamicLock() { releaseAndUnlock(); }
    void setAndLock(Mutex * m_)
    {   releaseAndUnlock(); 
        m = m_; 
        if (m)
            m->lock_I_understand_that_RAII_and_LockClass_are_better();
    }
    void setWithoutLock(Mutex * m_)
    {   releaseAndUnlock(); 
        m = m_; 
    }
    void releaseAndUnlock()
    {   if (m)
            m->unlock_I_understand_that_RAII_and_LockClass_are_better();
        m = 0;
    }
    void releaseWithoutUnlock() { m = 0; }

    //This has spurious wakeups. That is, it may return without a
    //corresponding notify_one or notify_all. (However, notify_one and
    //notify_all signals will not be "lost" with proper synchronization.) 
    friend void wait(DynamicLock& lock, CondVar& cond) { wait_I_understand_that_RAII_and_LockClass_are_better(*lock.m, cond); } 

private:
    DynamicLock(DynamicLock const& ); //not defined, not copyable
    DynamicLock& operator= (DynamicLock const& ); //not defined, not copyable
    Mutex * m;
};


class Thread
{
public:
    struct Runnable { virtual ~Runnable(){} virtual void run() = 0; };

    //Do not rely upon integral values. Use only the names. 
    enum DtorType { JoinInDtor = 1, DetachInDtor, AbortInDtor };

    //Will take ownership over runnable. Will invoke runnable->run() in the
    //new thread, and when run() returns, will destroy runnable with delete. 
    //If the thread is not joined or detached when the Thread object's
    //destructor is called (Thread::~Thread), then the second argument
    //specifies the action taken. 
    Thread(Runnable* runnable, DtorType dtorType);

    //Will make a copy of callable, and invoke (copy)() in the new thread.
    //If the thread is not joined or detached when the Thread object's
    //destructor is called (Thread::~Thread), then the second argument
    //specifies the action taken. 
    template <typename callable_t>
    Thread(callable_t callable, DtorType dtorType); 

    ~Thread(); 

    //Once joined, may not detach.
    //May call join multiple times, including concurrently. 
    //All threads calling join will block until the thread is done. 
    //A join call on a joined thread is a no-op. 
    void join() { Lock g(mutex); joinImpl(g); }

    //Once detached, may not join. 
    //May call detach multiple times, including concurrently. 
    //A detach call on a detached thread is a no-op.
    void detach() { Lock g(mutex); detachImpl(); }
    
private:
    Thread(Thread const& ); //not defined, not copyable
    Thread& operator= (Thread const& ); //not defined, not copyable

    void start(Runnable* runnable);
    void joinImpl(Lock & );
    void detachImpl();

    DtorType dtorType;

    enum { joinable, joining, joined, detached } state;

    Mutex mutex;

    #ifdef _WIN32
        HANDLE threadHandle;
    #else
        CondVar joinCond;
        pthread_t thread;
    #endif

private:
    template <typename callable_t>
    struct RunnableTemplate : Runnable
        {   RunnableTemplate(callable_t callable_) : callable(callable_) {}
            virtual void run() { callable(); }
            callable_t callable;
        };
};


class ThreadPool
{
public:
    ThreadPool(int numThreads);
    ~ThreadPool();

    void waitUntilIdle();

    //Always take ownership. 
    void addTask(Thread::Runnable* ); 

    //will not try to interrupt already running tasks
    void setStopFlag();
    
private:
    ThreadPool(ThreadPool const& ); //not defined, not copyable
    ThreadPool& operator= (ThreadPool const& ); //not defined, not copyable

    class WorkerMain;

    Mutex mutex;
    CondVar newTaskCondition;
    CondVar idleCondition;
    bool stopflag;
    int numRunningTasks;
    std::vector<Thread*> createdThreads;
    std::vector<Thread::Runnable*> pendingTasks;
};


// **** **** 
// Private Implementation

inline Mutex::Mutex()
{
#ifdef _WIN32
    InitializeCriticalSection(&criticalSection);
#else
    int x = pthread_mutex_init(&mutex, 0);
    if (x) JFATAL(x, 0);
#endif
}

inline Mutex::~Mutex()
{
#ifdef _WIN32
    DeleteCriticalSection(&criticalSection);
#else
    int x = pthread_mutex_destroy(&mutex);
    if (x) JFATAL(x, 0);
#endif
}

inline void Mutex::lock_I_understand_that_RAII_and_LockClass_are_better()
{
#ifdef _WIN32
    EnterCriticalSection(&criticalSection);
#else
    int x = pthread_mutex_lock(&mutex);
    if (x) JFATAL(x, 0);
#endif
}

inline void Mutex::unlock_I_understand_that_RAII_and_LockClass_are_better()
{
#ifdef _WIN32
    LeaveCriticalSection(&criticalSection);
#else
    int x = pthread_mutex_unlock(&mutex);
    if (x) JFATAL(x, 0);
#endif
}


//condition variable impl
#ifdef _WIN32
    inline CondVar::CondVar() { InitializeConditionVariable( & conditionVariable); }
    inline CondVar::~CondVar() {} 
    inline void wait_I_understand_that_RAII_and_LockClass_are_better(Mutex& m, CondVar& c)
    {
        SetLastError(0); 
        if (0 != SleepConditionVariableCS( & c.conditionVariable, & m.criticalSection, INFINITE))
            return; 
        DWORD const lastError = GetLastError(); 
        JFATAL(lastError, 0); 
    }
    inline void CondVar::notify_one() { WakeConditionVariable( & conditionVariable); }
    inline void CondVar::notify_all() { WakeAllConditionVariable( & conditionVariable); }
#else
    inline CondVar::CondVar()
        {   int x = pthread_cond_init(&pthreadCond, 0);
            if (x) JFATAL(x, 0);
        }
    inline CondVar::~CondVar()
        {   int x = pthread_cond_destroy(&pthreadCond);
            if (x) JFATAL(x, 0); }
    inline void wait_I_understand_that_RAII_and_LockClass_are_better(Mutex& m, CondVar& c)
        {   int x = pthread_cond_wait(&c.pthreadCond, &m.mutex);
            if (x) JFATAL(x, 0);
        }
    inline void CondVar::notify_one()
        {   int x = pthread_cond_signal(&pthreadCond); 
            if (x) JFATAL(x, 0);
        }
    inline void CondVar::notify_all()
        {   int x = pthread_cond_broadcast(&pthreadCond);
            if (x) JFATAL(x, 0);
        }
#endif


inline Thread::Thread(Runnable* runnable, DtorType dtorType_)
    : dtorType(dtorType_), state(joinable)
{   start(runnable);
}

template <typename callable_t>
inline Thread::Thread(callable_t callable, DtorType dtorType_)
    : dtorType(dtorType_), state(joinable)
{   start(new RunnableTemplate<callable_t>(callable));
}


}//namespace jjm

#endif
