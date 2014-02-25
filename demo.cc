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
