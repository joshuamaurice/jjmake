// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#include "jthreading.hpp"

#include "jbase/jfatal.hpp"
#include "jbase/juniqueptr.hpp"
#include <memory>

#ifdef _WIN32
    #include "process.h"
#endif

using namespace jjm;
using namespace std;


jjm::Thread::~Thread()
{   Lock g(mutex);
    if (JoinInDtor == dtorType)
        joinImpl(g);
    else if (DetachInDtor == dtorType)
        detachImpl();
    else if (AbortInDtor == dtorType)
    {   if (detached != state && joined != state)
            JFATAL(state, 0);
    }else
        JFATAL(dtorType, 0);
#ifdef _WIN32
    if (0 != threadHandle && ! CloseHandle(threadHandle))
        JFATAL(0, 0);
#endif
}

void jjm::Thread::detachImpl()
{   if (detached == state)
        return;
    if (joinable != state)
        JFATAL(state, 0);
#ifdef _WIN32
    if (!CloseHandle(threadHandle))
        JFATAL(0, 0);
    threadHandle = 0;
#else
    int const x = pthread_detach(thread);
    if (x)
        JFATAL(x, 0);
#endif
    state = detached;
}

void jjm::Thread::joinImpl(Lock & g)
{   
#ifdef _WIN32
    if (state == joinable || state == joining)
    {   state = joining;
        {   ReverseLock unsynchronized(g);
            for (;;)
            {   DWORD exitCode;
                if (!GetExitCodeThread(threadHandle, &exitCode))
                    JFATAL(0, 0);
                if (STILL_ACTIVE != exitCode)
                    break;
                DWORD const waitResult = WaitForSingleObject(threadHandle, INFINITE);
                if (WAIT_OBJECT_0 != waitResult && WAIT_TIMEOUT != waitResult)
                    JFATAL(waitResult, 0);
            }
        }
        state = joined;
    }else if (state == joined)
        ;
    else
        JFATAL(state, 0);
#else
    if (state == joinable)
    {   state = joining;
        {   ReverseLock unsynchronized(g);
            void * thread_return_val;
            int const x = pthread_join(thread, &thread_return_val);
            if (x) JFATAL(x, 0);
        }
        state = joined;
        joinCond.notify_all();
    }else if (state == joining)
    {   while (state == joining)
            g.wait(joinCond);
    }else if (state == joined)
        ;
    else
        JFATAL(state, 0);
#endif
}

#ifdef _WIN32
    extern "C" unsigned int __stdcall jjEntryPointForThreadsWin32(void* x)
#else
    extern "C" void* jjEntryPointForThreadsPosix(void* x)
#endif
    {   
        std::auto_ptr<jjm::Thread::Runnable>  runnable(static_cast<jjm::Thread::Runnable*>(x));
        runnable->run();
        return 0;
    }

void jjm::Thread::start(Runnable* runnable)
{
    auto_ptr<Runnable> runObject(runnable);
#ifdef _WIN32
    uintptr_t const tmpThreadHandle = _beginthreadex(
            0, //default security attributes
            0, //default stack size
            & jjEntryPointForThreadsWin32,
            runObject.get(),
            0, //make it start running, alternative is to create thread in suspended state
            0 //out-param for threadid
            ); 
    threadHandle = reinterpret_cast<HANDLE>(tmpThreadHandle);
    if (0 == threadHandle)
        JFATAL(0, 0);
#else
    int x = pthread_create(& thread,
                           0,
                           & jjEntryPointForThreadsPosix,
                           runObject.get());
    if (x)
        JFATAL(x, 0);
#endif
    runObject.release();
}

void jjm::sleep(unsigned long millisec)
{
#ifdef _WIN32
    Sleep(millisec);
#else
    usleep(millisec * 1000);
#endif
}


class jjm::ThreadPool::WorkerMain
{   
public:
    ThreadPool * pool;

    void operator() ()
    {
        Lock g(pool->mutex);
        for (;;)
        {   UniquePtr<Thread::Runnable*> task;
            for (;;)
            {   if (pool->stopflag)
                    return;
                if (pool->pendingTasks.size())
                {   task.reset(pool->pendingTasks.back());
                    pool->pendingTasks.pop_back();
                    break;
                }
                if (0 == pool->numRunningTasks)
                    pool->idleCondition.notify_all();
                wait(g, pool->newTaskCondition);
            }
            ++pool->numRunningTasks;
            {
                ReverseLock rg(g);
                task->run();
            }
            --pool->numRunningTasks;
        }
    }
};

jjm::ThreadPool::ThreadPool(int numThreads)
    : stopflag(false), numRunningTasks(0)
{
    WorkerMain workerMain;
    workerMain.pool = this;
    for (int i = 0; i < numThreads; ++i)
    {   createdThreads.push_back(0);
        createdThreads.back() = new Thread(workerMain, Thread::JoinInDtor);
    }
}

jjm::ThreadPool::~ThreadPool()
{
    setStopFlag();
    for (size_t i=0; i<createdThreads.size(); ++i)
        delete createdThreads[i];
}

void jjm::ThreadPool::waitUntilIdle()
{
    Lock g(mutex);
    for (;;)
    {   if (numRunningTasks == 0 && pendingTasks.size() == 0)
            return; 
        wait(g, idleCondition); 
    }
}

void jjm::ThreadPool::addTask(Thread::Runnable* task)
{
    UniquePtr<Thread::Runnable*> task2(task);
    {
        Lock g(mutex);
        pendingTasks.push_back(0);
        pendingTasks.back() = task2.release();
    }
    newTaskCondition.notify_one();
}

void jjm::ThreadPool::setStopFlag()
{
    {
        Lock g(mutex);
        stopflag = true;
    }
    newTaskCondition.notify_all();
}
