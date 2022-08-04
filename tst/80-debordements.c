#include "thread.h"
#include <stdint.h>
#include <assert.h>

void *toto(void *id) {
    int a[1000];
    a[0] = 1;
    if(id)
        return toto(id-a[0]);
    return NULL;
}

int main(int argc, char const *argv[])
{
    thread_t th1;
    void *tmp_ptr = (void*) (uintptr_t )10000;
    thread_create(&th1, toto, tmp_ptr);
    void *retval;
    int ret = thread_join(th1, &retval);
    assert(ret == -1);
    return 0;
}


