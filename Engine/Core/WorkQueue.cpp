//
// Urho3D Engine
// Copyright (c) 2008-2011 Lasse ��rni
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "Precompiled.h"
#include "ProcessUtils.h"
#include "WorkerThread.h"
#include "WorkItem.h"
#include "WorkQueue.h"

#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

/// Work queue implementation.
class WorkQueueImpl
{
public:
    /// Construct.
    WorkQueueImpl()
    {
        #ifdef WIN32
        eventHandle_ = CreateEvent(0, FALSE, FALSE, 0);
        #else
        pthread_cond_init(&condition_, 0);
        pthread_mutex_init(&mutex_, 0);
        #endif
    }
    
    /// Destruct.
    ~WorkQueueImpl()
    {
        #ifdef WIN32
        CloseHandle(eventHandle_);
        #else
        pthread_cond_destroy(&condition_);
        pthread_mutex_destroy(&mutex_);
        #endif
    }
    
    /// Signal one worker thread for available work.
    void Signal()
    {
        #ifdef WIN32
        SetEvent(eventHandle_);
        #else
        pthread_cond_signal(&condition_);
        #endif
    }
    
    /// Wait for available work.
    void Wait()
    {
        #ifdef WIN32
        WaitForSingleObject(eventHandle_, INFINITE);
        #else
        pthread_cond_wait(&condition_, &mutex_);
        #endif
    }
    
private:
    #ifdef WIN32
    /// Event for signaling the worker threads.
    HANDLE eventHandle_;
    #else
    /// Condition variable for signaling the worker threads.
    pthread_cond_t condition_;
    /// Mutex for the condition variable.
    pthread_mutex_t mutex_;
    #endif
};

OBJECTTYPESTATIC(WorkQueue);

WorkQueue::WorkQueue(Context* context) :
    Object(context),
    impl_(new WorkQueueImpl()),
    started_(false),
    shutDown_(false)
{
    // Create worker threads and start them
    unsigned numCores = GetNumCPUCores();
    for (unsigned i = 1; i < numCores; ++i)
    {
        SharedPtr<WorkerThread> thread(new WorkerThread(this, i));
        thread->Start();
        threads_.Push(thread);
    }
}

WorkQueue::~WorkQueue()
{
    // Stop the worker threads. First make sure they are not waiting for work items.
    if (!threads_.Empty())
    {
        shutDown_ = true;
        started_ = false;
        
        for (unsigned i = 0; i < threads_.Size(); ++i)
            impl_->Signal();
        for (unsigned i = 0; i < threads_.Size(); ++i)
            threads_[i]->Stop();
    }
    
    delete impl_;
    impl_ = 0;
}

void WorkQueue::AddWorkItem(WorkItem* item)
{
    if (!started_)
        queue_.Push(item);
    else
    {
        queueLock_.Acquire();
        queue_.Push(item);
        queueLock_.Release();
        impl_->Signal();
    }
}

void WorkQueue::Start()
{
    if (!threads_.Empty() && !started_)
    {
        started_ = true;
        for (unsigned i = 0; i < threads_.Size(); ++i)
            impl_->Signal();
    }
}

void WorkQueue::Finish()
{
    if (threads_.Empty())
    {
        // If no worker threads, do all work now in the main thread
        for (List<WorkItem*>::Iterator i = queue_.Begin(); i != queue_.End(); ++i)
            (*i)->Process(0);
        queue_.Clear();
    }
    else
    {
        // Wait for work to finish while also taking work items in the main thread
        for (;;)
        {
            WorkItem* item = 0;
            bool allIdle = false;
            
            queueLock_.Acquire();
            if (!queue_.Empty())
            {
                item = queue_.Front();
                queue_.PopFront();
            }
            else
                allIdle = CheckIdle();
            queueLock_.Release();
            
            if (item)
                item->Process(0);
            else if (allIdle)
                break;
        }
    }
}

void WorkQueue::FinishAndStop()
{
    Finish();
    started_ = false;
}

bool WorkQueue::IsCompleted()
{
    queueLock_.Acquire();
    bool queueEmpty = queue_.Empty();
    bool allIdle = false;
    if (queueEmpty)
        allIdle = CheckIdle();
    queueLock_.Release();
    
    return queueEmpty && allIdle;
}

WorkItem* WorkQueue::GetNextWorkItem(WorkerThread* thread)
{
    for (;;)
    {
        if (shutDown_)
            return 0;
        
        WorkItem* item = 0;
        queueLock_.Acquire();
        if (started_ && !queue_.Empty())
        {
            item = queue_.Front();
            queue_.PopFront();
            thread->SetWorking(true);
        }
        queueLock_.Release();
        
        if (item)
            return item;
        else
        {
            thread->SetWorking(false);
            impl_->Wait();
        }
    }
}

bool WorkQueue::CheckIdle()
{
    bool allIdle = true;
    for (unsigned i = 0; i < threads_.Size(); ++i)
    {
        if (threads_[i]->IsWorking())
        {
            allIdle = false;
            break;
        }
    }
    return allIdle;
}