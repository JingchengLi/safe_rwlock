#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

enum {
    NO_LOCK = 0,
    READ_LOCK,
    WRITE_LOCK
} rwlock_type;

pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;
pthread_once_t once = PTHREAD_ONCE_INIT;
pthread_key_t rwlock_status;

void destructor(void *ptr)  
{  
    free(ptr);
}  

void init_once(void)  
{  
    pthread_key_create(&rwlock_status, destructor);  
}  

int __safe_rwlock_lock(pthread_rwlock_t* lock, int lock_type)
{
    int ret;
    int *ptr = NULL;
    if (READ_LOCK != lock_type && WRITE_LOCK != lock_type)
    {
        ret = -1;
        return ret;
    }
    printf(">>>>>>>>>>>>>>> %s lock begin\n", READ_LOCK == lock_type ? "read" : "write");
    ptr = pthread_getspecific(rwlock_status);
    if (NULL == ptr)
    {
        ret = EINVAL;
        return ret; // rwlock is not initialised in this thread.
    }
    if (NO_LOCK != *ptr)
    {
        printf("EDEADLK \n");
        ret = EDEADLK; 
        return ret;
    }

    if (READ_LOCK == lock_type)
    {
        ret = pthread_rwlock_tryrdlock(lock);
    }
    else
    {
        ret = pthread_rwlock_trywrlock(lock);
    }
    if (0 == ret)
    {
        *ptr = lock_type;
        pthread_setspecific(rwlock_status, ptr);
        /* Get a lock successfully. */        
        printf("%s lock success\n", READ_LOCK == lock_type ? "read" : "write");
    }
    else if (EBUSY == ret)
    {
        printf("wait write lock\n");

        if (READ_LOCK == lock_type)
        {
            ret = pthread_rwlock_rdlock(lock);
        }
        else
        {
            ret = pthread_rwlock_wrlock(lock);
        }
        if (EDEADLK == ret)
        {
            /* Already hold a lock. */
            printf("EDEADLK \n");
        }
        else if (0 == ret)
        {
            *ptr = lock_type;
            pthread_setspecific(rwlock_status, ptr);
            printf("%s lock success\n", READ_LOCK == lock_type ? "read" : "write");
        }
        else
        {
            printf("%s lock failed\n", READ_LOCK == lock_type ? "read" : "write");
        }
    }
    else
    {
        printf("%s lock failed\n", READ_LOCK == lock_type ? "read" : "write");
    }
    printf(">>>>>>>>>>>>>>> %s lock end\n\n", READ_LOCK == lock_type ? "read" : "write");

    return ret;    
}

/* return value: 0 when success, other values when fail to lock. */
int safe_rwlock_wrlock(pthread_rwlock_t* lock)
{
    return __safe_rwlock_lock(lock, WRITE_LOCK);
}

/* return value: 0 when success, other values when fail to lock. */
int safe_rwlock_rdlock(pthread_rwlock_t* lock)
{
    return __safe_rwlock_lock(lock, READ_LOCK);
}

void safe_rwlock_unlock(pthread_rwlock_t* lock)
{
    int *ptr = NULL;
    ptr = pthread_getspecific(rwlock_status);
    if (NO_LOCK != *ptr)
    {
        pthread_rwlock_unlock(lock);
        printf("release %s lock\n", READ_LOCK == *ptr ? "read" : "write");
        *ptr = NO_LOCK;
        pthread_setspecific(rwlock_status, ptr);
    }
}

/*  ADD THIS BLOCK TO THE START OF EVERY THREAD FUNCTION USE RWLOCK */
void init_rwlock_status()
{
    int* ptr = NULL;
    pthread_once(&once, init_once);
    if ((ptr = pthread_getspecific(rwlock_status)) == NULL)
    {
        ptr = malloc(sizeof(int));
        *ptr = NO_LOCK;
        pthread_setspecific(rwlock_status, ptr);
    }
}

void *safe_func(void *arg)
{
    init_rwlock_status();
    while (1) {
        printf("begin\n");
        printf("1\n");
        safe_rwlock_rdlock(&rwlock);
        printf("2\n");
        safe_rwlock_wrlock(&rwlock);
        printf("3\n");
        safe_rwlock_wrlock(&rwlock);
        printf("4\n");
        safe_rwlock_rdlock(&rwlock);
        printf("5\n");
        safe_rwlock_rdlock(&rwlock);

        safe_rwlock_unlock(&rwlock);
        safe_rwlock_unlock(&rwlock);
        safe_rwlock_unlock(&rwlock);
        safe_rwlock_unlock(&rwlock);
        safe_rwlock_unlock(&rwlock);
        printf("end\n\n\n");
    }
}


void *func(void *arg)
{
    while (1) {
        printf("begin\n");
        printf("1\n");
        pthread_rwlock_wrlock(&rwlock);
        printf("2\n");
        pthread_rwlock_rdlock(&rwlock);

        pthread_rwlock_unlock(&rwlock);
        pthread_rwlock_unlock(&rwlock);
        printf("end\n\n\n");
    }
}


int main()
{
    pthread_t thd;
    pthread_create(&thd, NULL, safe_func, NULL);
    //pthread_create(&thd, NULL, func, NULL);

    pthread_key_delete(rwlock_status);  

    sleep(100);
}
