#ifndef KOLTHREAD_H_INCLUDED
#define KOLTHREAD_H_INCLUDED

/* Note on Windows
 * <winsock2.h> is used instead of <windows.h> because
 * that the <windows.h> includes the <winsock.h> if
 * <winsock2.h> is not included.
*/
#ifdef WIN32
#include <winsock2.h>
#include <process.h>
#define SEM_VALUE_MAX   (2147483647)
#else
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#endif /* WIN32 */

namespace kol
{
#ifdef WIN32
  typedef HANDLE mutex_t;
  typedef HANDLE thread_t;
  typedef HANDLE sem_t;
#else
  typedef pthread_mutex_t mutex_t;
  typedef pthread_t thread_t;
#endif /* WIN32 */

  class Mutex
  {
  public:
    Mutex();
    virtual ~Mutex();
    virtual int lock();
    virtual int trylock();
    virtual int unlock();

  protected:
    mutex_t m_mutex;
  };

  class Semaphore
  {
  public:
    Semaphore(unsigned int value);
    virtual ~Semaphore();
    int wait();
    int trywait();
    int post();

  protected:
    sem_t m_sem;
  };

  class ThreadController;
  class Thread
  {
  private:
#ifdef WIN32
    static unsigned int __stdcall start_routine(void *);
#else
    static void* start_routine(void *);
#endif

  public:
    static void millisleep(unsigned long msec);

  public:
    Thread();
    virtual ~Thread();
    virtual int start();
    virtual int join();
    virtual int cancel();
    void controller(ThreadController* pctrl);
    ThreadController* controller() const;
    void delthreadid(thread_t t);
    thread_t delthreadid() const;
   
  protected:
    virtual int run();
   
  protected:
    ThreadController* m_controller;
    thread_t m_threadid;
    thread_t m_delthreadid;
  };

  class ThreadController
  {
  private:
#ifdef WIN32
    static unsigned int __stdcall ctrl_routine(void *);
#else
    static void* ctrl_routine(void *);
#endif

  public:
    ThreadController();
    virtual ~ThreadController();
    bool post(Thread* p);
    bool done(Thread* p);
    int lock();
    int unlock();
    int join();
    int numrunning();
    void delthread(Thread* p);

  private:
    void closeid();

  private:
    int m_running;
    thread_t m_threadid;
    Mutex m_mutex;
  };
}
#endif

