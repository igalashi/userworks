// kolthread.cxx
//
#include "kolthread.h"

using namespace kol;

//
// Definitions for Mutex
//

Mutex::Mutex()
{
#ifdef WIN32
  m_mutex = ::CreateMutex(0,0,0);
#else
  ::pthread_mutex_init(&m_mutex,0);
#endif /* WIN32 */
}

Mutex::~Mutex()
{
#ifdef WIN32
  ::CloseHandle(m_mutex);
#else
  ::pthread_mutex_destroy(&m_mutex);
#endif
}

int
Mutex::lock()
{
#ifdef WIN32
  if(::WaitForSingleObject(m_mutex,INFINITE) == WAIT_OBJECT_0)
    return 0;
  return (-1);
#else
  return ::pthread_mutex_lock(&m_mutex);
#endif
}

int
Mutex::unlock()
{
#ifdef WIN32
  if(::ReleaseMutex(m_mutex))
    return 0;
  return (-1);
#else
  return ::pthread_mutex_unlock(&m_mutex);
#endif
}

int
Mutex::trylock()
{
#ifdef WIN32
  if(::WaitForSingleObject(m_mutex,0) == WAIT_OBJECT_0)
    return 0;
  return (-1);
#else
  return ::pthread_mutex_trylock(&m_mutex);
#endif
}

//
// Definitions for Semaphore
//

Semaphore::Semaphore(unsigned int value)
{
#ifdef WIN32
  m_sem = ::CreateSemaphore( 0, (LONG)value, SEM_VALUE_MAX, 0 );
#else
  ::sem_init(&m_sem, 0, value);
#endif
}

Semaphore::~Semaphore()
{
#ifdef WIN32
  ::CloseHandle(m_sem);
#else
  ::sem_destroy(&m_sem);
#endif
}

int
Semaphore::wait()
{
#ifdef WIN32
  if(::WaitForSingleObject(m_sem,INFINITE) == WAIT_OBJECT_0)
    return 0;
  return (-1);
#else
  return ::sem_wait(&m_sem);
#endif
}

int
Semaphore::trywait()
{
#ifdef WIN32
  if(::WaitForSingleObject(m_sem,0) == WAIT_OBJECT_0)
    return 0;
  return (-1);
#else
  return ::sem_trywait(&m_sem);
#endif
}

int
Semaphore::post()
{
#ifdef WIN32
  if(::ReleaseSemaphore(m_sem, 1, 0))
    return 0;
  return (-1);
#else
  return ::sem_post(&m_sem);
#endif
}

//
// Definitions for Thread
//
 
Thread::Thread()
{
  m_controller = 0;
  m_threadid = 0;
  m_delthreadid = 0;
}
 
Thread::~Thread()
{
#ifdef WIN32
  if( m_threadid )
    ::CloseHandle( m_threadid );
#endif
}


#ifdef WIN32
unsigned int __stdcall
#else 
void*
#endif
Thread::start_routine(void* arg)
{
  if(arg == 0)
    return 0;
  Thread* p = (Thread*)arg;
  ThreadController* pctrl = p->controller();
  p->run();
  if( pctrl )
    pctrl->done( p );
#ifdef WIN32
  return 0;
#else
  return arg;
#endif
}

void
Thread::controller(ThreadController* pctrl)
{
  m_controller = pctrl;
}

ThreadController*
Thread::controller() const
{
  return m_controller;
}

void
Thread::delthreadid(thread_t t)
{
  m_delthreadid = t;
}

thread_t
Thread::delthreadid() const
{
  return m_delthreadid;
}

void
Thread::millisleep(unsigned long msec)
{
#ifdef WIN32
  ::Sleep((DWORD)msec);
#else
  struct timespec ts;
  ts.tv_sec = (time_t)(msec / 1000);
  ts.tv_nsec = (long)((msec % 1000) * 1000000);
  ::nanosleep( &ts, 0 );
#endif
}

int
Thread::start()
{
#ifdef WIN32
  unsigned threadaddr;
  m_threadid = (thread_t)::_beginthreadex(0, 0, Thread::start_routine, this, 0, &threadaddr);
  if(m_threadid == 0)
    return (-1);
  return 0;
#else
  return ::pthread_create(&m_threadid, 0, Thread::start_routine, this);
#endif
}
 
int
Thread::join()
{
  if( m_threadid == 0 )
    return (-1);
#ifdef WIN32
  if(::WaitForSingleObject(m_threadid, INFINITE) == WAIT_OBJECT_0)
    return 0;
  return (-1);
#else
  return ::pthread_join(m_threadid, 0);
#endif
}

int
Thread::cancel()
{
  if( m_threadid == 0 )
    return (-1);
#ifdef WIN32
  if(::TerminateThread(m_threadid,(DWORD)0))
    return 0;
  return (-1);
#else
  return ::pthread_cancel(m_threadid);
#endif
};
 
int
Thread::run()
{
  return 0;
}

//
// Definitions for ThreadController
//

ThreadController::ThreadController()
{
  m_running = 0;
  m_threadid = 0;
}

ThreadController::~ThreadController()
{
  while( numrunning() )
    Thread::millisleep( 100 );
  join();
  closeid();
}

#ifdef WIN32
unsigned int __stdcall
#else 
void*
#endif
ThreadController::ctrl_routine(void* arg)
{
  if(arg == 0)
    return 0;
  Thread* p = (Thread*)arg;
  ThreadController* pctrl = p->controller();
  if( pctrl == 0 )
    return 0;
  pctrl->delthread( p );
  return 0;
}

void
ThreadController::closeid()
{
  if( m_threadid == 0 )
    return;
#ifdef WIN32
  ::CloseHandle( m_threadid );
#endif
  m_threadid = 0;
}

void
ThreadController::delthread(Thread* p)
{
  m_mutex.lock();
  join();
  closeid();
  m_threadid = p->delthreadid();
  p->join();
  delete p;
  --m_running;
  m_mutex.unlock();
}

bool
ThreadController::post(Thread* p)
{
  if( p == 0 )
    return false;
  p->controller( this );
  p->start();
  m_mutex.lock();
  ++m_running;
  m_mutex.unlock();
  return true;
}

bool
ThreadController::done(Thread* p)
{
  if( p == 0 )
    return false;
  thread_t tid;
#ifdef WIN32
  unsigned threadaddr;
  tid = (thread_t)::_beginthreadex(0, 0, ThreadController::ctrl_routine, p, 0, &threadaddr);
#else
  if(::pthread_create(&tid, 0, ThreadController::ctrl_routine, p) != 0)
    tid = 0;
#endif
  p->delthreadid(tid);
  if( tid == 0 )
    return false;
  return true;
}

int
ThreadController::join()
{
  if( m_threadid == 0 )
    return (-1);
#ifdef WIN32
  if(::WaitForSingleObject(m_threadid, INFINITE) == WAIT_OBJECT_0)
    return 0;
  return (-1);
#else
  return ::pthread_join(m_threadid, 0);
#endif
}

int
ThreadController::lock()
{
  return m_mutex.lock();
}

int
ThreadController::unlock()
{
  return m_mutex.unlock();
}

int
ThreadController::numrunning()
{
  int n;
  m_mutex.lock();
  n = m_running;
  m_mutex.unlock();
  return n;
}
