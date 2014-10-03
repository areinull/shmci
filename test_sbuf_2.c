#include "shmsbuf.h"
#include <stdio.h>
#include <pthread.h>
#include <sys/sem.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#define LOOPS 1000u
#define NUMB 128
#define STOP 0xFFFFFFFF


static struct timespec timeout={.tv_sec=3,.tv_nsec=0};
static struct sembuf wrlock[] =
{
    {1, 0, 0},
    {1, 1, SEM_UNDO},
    {0, 0, 0}
};

void *consumer(void *ptr)
{
    pthread_t tid = pthread_self();
    struct ShmSBuf sbuf;
    unsigned int res = 0;
    int err;
    (void)ptr;
    shm_sbuf_init(&sbuf, 1, NUMB, sizeof (unsigned int));
    printf("Consumer %u connected - %s\n", (unsigned int)tid, sbuf.err_msg);

    while (1)
    {
        if ((err = shm_sbuf_last(&sbuf, &res)))
        {
            if (err != (int)SOME_ERR)
            {
                printf("[%u] shm_sbuf_last(): error - %s\n",
                       (unsigned int)tid, sbuf.err_msg);
            }
            continue;
        }
        if (res == STOP)
        {
            printf("[%u] Got stop\n", (unsigned int)tid);
            break;
        }
        if (res >= LOOPS)
        {
            printf("[%u] Consumer got magic number %u\n",
                   (unsigned int)tid, res);
        }
    }

    shm_sbuf_deinit(&sbuf);
    return NULL;
}

void *blocking_consumer(void *ptr)
{
    pthread_t tid = pthread_self();
    struct ShmSBuf sbuf;
    unsigned int res[NUMB];
    int err, i;
    (void)ptr;
    shm_sbuf_init(&sbuf, 1, NUMB, sizeof (unsigned int));
    printf("Blocking consumer %u connected - %s\n",
           (unsigned int)tid, sbuf.err_msg);

    while (1)
    {
        if ((err = shm_sbuf_nread(&sbuf, res, &timeout)) <= 0)
        {
            printf("[%u] shm_sbuf_nread(): error - %s\n",
                    (unsigned int)tid, sbuf.err_msg);
            if (err == (int)SEM_TIMEOUT)
                break;
            continue;
        }
        if (res[err-1] == STOP)
        {
            printf("[%u] Got stop\n", (unsigned int)tid);
            break;
        }
        for (i=0; i<err; ++i)
        {
            printf("%u ", res[i]);
            if (res[i] >= LOOPS)
                printf("\n[%u] Blocking consumer got magic number %u\n",
                       (unsigned int)tid, res[i]);
        }
        puts("");
    }

    shm_sbuf_deinit(&sbuf);
    return NULL;
}

void *producer(void *ptr)
{
    pthread_t tid = pthread_self();
    struct ShmSBuf sbuf;
    unsigned int loop;
    (void)ptr;
    shm_sbuf_init(&sbuf, 1, NUMB, sizeof (unsigned int));
    printf("Producer %u connected - %s\n", (unsigned int)tid, sbuf.err_msg);

    for (loop=0; loop<LOOPS; ++loop)
    {
        if (loop % (LOOPS/10))
        {
            if (shm_sbuf_add(&sbuf, &loop, 1))
            {
                printf("[%u] shm_sbuf_add(): error - %s\n",
                       (unsigned int)tid, sbuf.err_msg);
                continue;
            }
        }
        else
        {
            unsigned int idx=0, total=0, tmp=0;
            tmp = loop + LOOPS;
            if (semtimedop(sbuf.sem_id, wrlock, 3, &timeout) < 0)
            {
                printf("[%u] error locking write semaphore - %s",
                       (unsigned int)tid, strerror(errno));
                continue;
            }
            memcpy(&total, (char*)sbuf.ptr+OFF_CNT, sizeof total);
            memcpy(&idx, (char*)sbuf.ptr+OFF_IDX, sizeof idx);
            memcpy((char*)sbuf.ptr+sbuf.dataoffset+idx*sbuf.elsize,
                   (char*)&tmp, sbuf.elsize);
            ++idx;
            idx %= sbuf.nmemb;
            ++total;
            memcpy((char*)sbuf.ptr+OFF_CNT, &total, sizeof total);
            memcpy((char*)sbuf.ptr+OFF_IDX, &idx, sizeof idx);
            printf("[%u] Producer at loop %u with sleep\n",
                   (unsigned int)tid, loop);
            sleep(1);
            printf("[%u] Producer waked\n", (unsigned int)tid);
            if (semctl(sbuf.sem_id, 1, SETVAL, 0) < 0)
            {
                printf("[%u] error unlocking write semaphore - %s",
                       (unsigned int)tid, strerror(errno));
                continue;
            }
        }
    }
    loop = STOP;
    if (shm_sbuf_add(&sbuf, &loop, 1))
    {
        printf("[%u] shm_sbuf_add(): error - %s\n",
               (unsigned int)tid, sbuf.err_msg);
    }

    shm_sbuf_deinit(&sbuf);
    return NULL;
}

int main()
{
    pthread_t cons1, cons2, cons_b, prod;
    struct timeval tv1, tv2;

    gettimeofday(&tv1, NULL);

    pthread_create(&cons1, NULL, consumer, NULL);
    pthread_create(&cons2, NULL, consumer, NULL);
    pthread_create(&cons_b, NULL, blocking_consumer, NULL);
    pthread_create(&prod, NULL, producer, NULL);

    pthread_join(cons1, NULL);
    pthread_join(cons2, NULL);
    pthread_join(cons_b, NULL);
    pthread_join(prod, NULL);

    gettimeofday(&tv2, NULL);

    if (tv1.tv_usec > tv2.tv_usec)
    {
        tv2.tv_sec--;
        tv2.tv_usec += 1000000;
    }
    printf("Result time - %ld.%ld\n", tv2.tv_sec - tv1.tv_sec,
           tv2.tv_usec - tv1.tv_usec);

    return 0;
}
