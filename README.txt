what's threadpool
=================
threadpool is a very simple thread pool, simple implement, simple use.

how to use
==========
In threadpool, use object as executable unit. once object is submited to threadpool,
threadpool will put task in queue, and exec it.

First incluce "threadpool.h", and write a class inherit Task, implement the doit function.
In this function, do the job you want the threadpool to do.

Then construct a Threadpool, and submit the object to it.

# cat demo.cc

#include <cstdio>
#include <unistd.h>
#include <threadpool.h>

class PrintTask : public Task {
public:
    PrintTask(const char *s) : s_(s) { }
    bool doit() {
        printf("%s\n", s_);
        return true;
    }
private:
    const char *s_;
};

int main()
{
    ThreadPool tpool(10);
    
    PrintTask *t1 = new PrintTask("task one");
    tpool.submit(t1);

    PrintTask *t2 = new PrintTask("task two");
    tpool.submit(t2);

    tpool.quit();
    return 0;
}

# g++ -lthreadpool -lpthread -o demo demo.cc

api
===
/* @param tnum the thread number
 * @param maxTask the task pool's size, if exceed, submit will fail, default 1e6
 * @param attr thread attr
 */
ThreadPool(size_t tnum, size_t maxTask = 0,
           const pthread_attr_t *attr = NULL);

/* @param task the task object, ptr must be valid before exec
 * @param onfinished, when task finished call this
 *        typedef void (*OnTaskFinishedPtr)(Task *);
 * @return true if success 
 */
bool submit(Task *task, OnTaskFinishedPtr onfinished = NULL);

/* @return the number of task in the pool
 */
size_t taskNum();

/* @desc stop the pool, deny new task.
 *       quit will wait the tasks in the pool finished
 *       forceQuit stops without tasks finished
 *       ~ThreadPool will call forceQuit if none of the func is called
 */
bool quit();
bool forceQuit();

install
=======
yum install autoconf
yum install automake
yum install libtool
./autogen.sh && ./configure && make && make install && ldconfig
