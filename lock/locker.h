#ifndef LOCKER_H
#define LOCKER_H

#include <exception>
#include <pthread.h>
#include <semaphore.h>

// 封装 信号量 的类
class sem
{
public:
    sem()
    {
        if (sem_init(&m_sem, 0, 0) != 0)
        {
            throw std::exception();
        }
    }
    sem(int num)
    {
        if (sem_init(&m_sem, 0, num) != 0)
        {
            throw std::exception();
        }
    }
    ~sem()
    {
        sem_destroy(&m_sem);
    }
    bool wait()  // 以原子操作的方式将 信号量的值-1
    {
        return sem_wait(&m_sem) == 0;
    }
    bool post()    //  以原子操作的方式 将信号量的值 +1
    {
        return sem_post(&m_sem) == 0;
    }

private:
    sem_t m_sem;
};

// 封装 互斥锁  控制 对共享数据的访问
class locker
{
public:
    locker()
    {
        if (pthread_mutex_init(&m_mutex, NULL) != 0)
        {
            throw std::exception();
        }
    }
    ~locker()
    {
        pthread_mutex_destroy(&m_mutex);
    }
    bool lock()
    {
        return pthread_mutex_lock(&m_mutex) == 0;
    }
    bool unlock()
    {
        return pthread_mutex_unlock(&m_mutex) == 0;
    }
    pthread_mutex_t *get()
    {
        return &m_mutex;
    }

private:
    pthread_mutex_t m_mutex;
};


// 条件变量提供 线程间的一种通信进制。当某个 共享数据的值达到某个值时，唤醒等待这个变量的线程。
// 封装  条件变量
class cond
{
public:
    cond()
    {
        if (pthread_cond_init(&m_cond, NULL) != 0)
        {
            //pthread_mutex_destroy(&m_mutex);
            throw std::exception();
        }
    }
    ~cond()
    {
        pthread_cond_destroy(&m_cond);
    }
    bool wait(pthread_mutex_t *m_mutex)   
    {
        int ret = 0;
        //  m_mutex 用于保护条件变量的互斥锁。pthread_cond_wait 执行时，调用 线程放入条件变量的等待队列中，然后将 mutex 解锁.
        // 因为使用互斥锁，线程不会错过 目标变量的任何变化。
        //pthread_mutex_lock(&m_mutex);
        ret = pthread_cond_wait(&m_cond, m_mutex);          // 条件等待
       // pthread_mutex_unlock(&m_mutex);
        
        return ret == 0;
    }
    bool timewait(pthread_mutex_t *m_mutex, struct timespec t)  
    {
        int ret = 0;
        //pthread_mutex_lock(&m_mutex);
        ret = pthread_cond_timedwait(&m_cond, m_mutex, &t);         // 计时等待
        //pthread_mutex_unlock(&m_mutex);
        return ret == 0;
    }
    bool signal()
    {
        return pthread_cond_signal(&m_cond) == 0;                // 激活 一个等待该条件 的线程
    }
    bool broadcast()
    {
        return pthread_cond_broadcast(&m_cond) == 0;             // 激活 所有等待线程
    }

private:
    static pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;
};
#endif
