#include <iostream>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <sys/resource.h>
#include <threadpool.h>

// a simple but useful callback
void deleteTask(Task *task)
{
    delete task;
}

Task::~Task() {}

ThreadPool::ThreadPool(size_t tnum, size_t maxTask, const pthread_attr_t *attrp)
    : tnum_(tnum), maxTask_( maxTask != 0 ? maxTask : (size_t)(1e6))
{

    int rc = 0;

    pthread_cond_init(&taskCond_, NULL);
    pthread_mutex_init(&taskMutex_, NULL);

    runStat_ = Run;

    for (size_t i = 0; i < tnum_; ++i) {
        pthread_t ptid;
        if ((rc = pthread_create(&ptid, attrp, work, this)) != 0) {
            std::cerr << "create thread failed:" << errno << "\t"
                      << strerror(errno) << std::endl;
            runStat_ = Quit;
            break;
        }
        threadIds.push_back(ptid);
    }

    if (rc != 0) {
        threadsQuit(Run);
        throw rc;
    }
}

ThreadPool::~ThreadPool()
{
    if (runStat_ == Run) forceQuit();
    pthread_mutex_destroy(&taskMutex_);
    pthread_cond_destroy(&taskCond_);
}

bool ThreadPool::submit(Task *task, OnTaskFinishedPtr ptr)
{
    if (runStat_ != Run) return false;
    if (!task) return true;

    {
        SmartMutex mutex(&taskMutex_);

        if (tasks_.size() == maxTask_) return false;
        tasks_.push(makeOneTask(task, ptr));
    }

    pthread_cond_signal(&taskCond_);

    return true;
}

void *ThreadPool::work(void *e)
{
    ThreadPool *me = static_cast<ThreadPool*>(e);
    return me->work();
}

void *ThreadPool::work()
{
    OneTask task;
    while (true) {
        {
            SmartMutex mutex(&taskMutex_);
            while (tasks_.size() == 0) { 
                if (runStat_ != Run) return NULL;
                pthread_cond_wait(&taskCond_, &taskMutex_);
            }
            task = tasks_.front();
            tasks_.pop();
        }

        task.task->doit();
        if (task.onfinished) task.onfinished(task.task);

        if (runStat_ == ForceQuit) return NULL;
    }

    return NULL;
}

void ThreadPool::threadsQuit(RunStat stat) {
    if (stat != Run) {
        {
            SmartMutex mutex(&taskMutex_);
            runStat_ = stat;
        }
        pthread_cond_broadcast(&taskCond_);
    }
    for (std::vector<pthread_t>::iterator ite = threadIds.begin(),
            end = threadIds.end(); ite != end; ++ite) {
        void *ret;
        pthread_join(*ite, &ret);
    }
}

void setMaxfd(int *maxfd)
{
    struct rlimit rl; 

    if (getrlimit(RLIMIT_NOFILE, &rl) < 0) {
        printf("can't get file limit");
    } else {
        rl.rlim_cur = rl.rlim_max = *maxfd; 
        if (rl.rlim_cur > rl.rlim_max)
            *maxfd = rl.rlim_cur = rl.rlim_max;
        if (setrlimit(RLIMIT_NOFILE, &rl) < 0) {
            printf("setrlimit() error!");
        }       
    }       
}
