#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include <queue>
#include <pthread.h>

class Task {
public:
    virtual ~Task() = 0;
    virtual bool doit() { return true; };
};

class SmartMutex {
public:
    SmartMutex(pthread_mutex_t *mutex) {
        pthread_mutex_lock(mutex);
        mutex_ = mutex;
    }
    ~SmartMutex() {
        pthread_mutex_unlock(mutex_);
    }
private:
    pthread_mutex_t *mutex_;
};

typedef void (*OnTaskFinishedPtr)(Task *);
void deleteTask(Task *);

class ThreadPool {
public:
    ThreadPool(size_t tnum, size_t maxTask = 0, const pthread_attr_t *attr = NULL);
    ~ThreadPool();

public:
    bool submit(Task *task, OnTaskFinishedPtr onfinished = NULL);
    size_t taskNum() const { return tasks_.size(); } 
    
    static void *work(void *);
    void *work();

    bool quit() {
        threadsQuit(Quit);
        return true;
    }
    bool forceQuit() {
        threadsQuit(ForceQuit);
        return true;
    }

private:
    typedef enum {Run, Quit, ForceQuit} RunStat;

    ThreadPool(const ThreadPool &);
    ThreadPool &operator=(const ThreadPool &);
    void threadsQuit(RunStat stat);

private:    
    struct OneTask {
        Task *task;
        OnTaskFinishedPtr onfinished;
    };
    OneTask makeOneTask(Task *t, OnTaskFinishedPtr ptr) {
        OneTask task = {t, ptr};;
        return task;
    }

private:
    RunStat runStat_;

    size_t tnum_;
    std::vector<pthread_t> threadIds;

    std::queue<OneTask> tasks_;
    size_t maxTask_;

    pthread_cond_t taskCond_;
    pthread_mutex_t taskMutex_;
};

void setMaxfd(int *maxfd);

#endif 
