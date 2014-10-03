#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include "shmsbuf.h"

#define SHMSBUF_UNIQ_FILE "/usr/local/include/shmci/shmsbuf.h"

#ifndef _FILE_
#define _FILE_ (strrchr(__FILE__,'/') ? strrchr(__FILE__,'/')+1 : __FILE__ )
#endif

static struct timespec timeout={.tv_sec=3,.tv_nsec=0};

static struct sembuf rdlock[] =
{
    {1, 0, 0},
    {0, 1, SEM_UNDO}
};

static struct sembuf rdunlock[] =
{
    {0, -1, SEM_UNDO}
};

static struct sembuf wrlock[] =
{
    {1, 0, 0},
    {1, 1, SEM_UNDO},
    {0, 0, 0}
};

static struct sembuf wakelock[] =
{
    {2, 1, 0}
};

static struct sembuf wakewait[] =
{
    {2, 0, 0}
};

/* -------------------------------------------------------------------------- */
static int shm_init(struct ShmSBuf *cont, int proj_id, size_t size)
{
    char semflag = 0;
    key_t key;
    struct shmid_ds shm_desc;
    memset(&shm_desc, 0, sizeof shm_desc);

    if ((key = ftok(SHMSBUF_UNIQ_FILE, proj_id)) < 0)
    {
        snprintf(cont->err_msg, SHMSBUF_EMSG_SIZE,
                 "[%s:%d] shm_init(): ftok() call failed - %s",
                 _FILE_, __LINE__, strerror(errno));
        return -1;
    }
    /* Try to create shm segment */
    if ((cont->shm_id = shmget(key, size, 0666|IPC_CREAT|IPC_EXCL)) < 0)
    {
        if (errno == EEXIST)   /* Shm segment for this key exists already */
        {
            /* Get ID of existing seg */
            if ((cont->shm_id = shmget(key, 1, 0666)) < 0)
            {
                snprintf(cont->err_msg, SHMSBUF_EMSG_SIZE,
                         "[%s:%d] shm_init(): shmget() call failed - %s",
                         _FILE_, __LINE__, strerror(errno));
                return -2;
            }
            if (shmctl(cont->shm_id, IPC_STAT, &shm_desc) < 0)
            {
                snprintf(cont->err_msg, SHMSBUF_EMSG_SIZE,
                         "[%s:%d] shm_init(): shmctl() call failed - %s",
                         _FILE_, __LINE__, strerror(errno));
                return -3;
            }
            /* Segment unused - delete it and create anew */
            if (shm_desc.shm_nattch == 0)
            {
                semflag = 1;        /* Flag - recreate semaphore */
                if (shmctl(cont->shm_id, IPC_RMID, NULL) < 0)
                {
                    snprintf(cont->err_msg, SHMSBUF_EMSG_SIZE,
                             "[%s:%d] shm_init(): shmctl() call failed - %s",
                             _FILE_, __LINE__, strerror(errno));
                    return -4;
                }
                if ((cont->shm_id = shmget(key, size, 0666|IPC_CREAT)) < 0)
                {
                    snprintf(cont->err_msg, SHMSBUF_EMSG_SIZE,
                             "[%s:%d] shm_init(): shmget() call failed - %s",
                             _FILE_, __LINE__, strerror(errno));
                    return -5;
                }
                cont->shm_size = size;
            }
            else /* Segment is already in use */
            {
                cont->shm_size = shm_desc.shm_segsz;
            }
        }
        else
        {
            snprintf(cont->err_msg, SHMSBUF_EMSG_SIZE,
                     "[%s:%d] shm_init(): shmget() call failed - %s",
                     _FILE_, __LINE__, strerror(errno));
            return -6;
        }
    }
    else /* Segment created */
    {
        cont->shm_size = size;
    }
    /* Attach new shm segment to address space */
    if ((cont->ptr = shmat(cont->shm_id, NULL, 0)) == (void*)-1)
    {
        snprintf(cont->err_msg, SHMSBUF_EMSG_SIZE,
                 "[%s:%d] shm_init(): shmat() call failed - %s",
                 _FILE_, __LINE__, strerror(errno));
        return -7;
    }
    /* Try to create semaphore set */
    if ((cont->sem_id = semget(key, 3, 0666|IPC_CREAT|IPC_EXCL)) < 0)
    {
        if (errno == EEXIST) /* Semaphore set for this key exists already */
        {
            /* Get ID of existing sem */
            if ((cont->sem_id = semget(key, 3, 0666)) < 0)
            {
                snprintf(cont->err_msg, SHMSBUF_EMSG_SIZE,
                         "[%s:%d] shm_init(): semget() call failed - %s",
                         _FILE_, __LINE__, strerror(errno));
                shmdt(cont->ptr);
                return -10;
            }
            if (semflag)
            {
                if (semctl(cont->sem_id, 0, IPC_RMID) < 0)
                {
                    snprintf(cont->err_msg, SHMSBUF_EMSG_SIZE,
                             "[%s:%d] shm_init(): semctl() call failed - %s",
                             _FILE_, __LINE__, strerror(errno));
                    shmdt(cont->ptr);
                    return -11;
                }
                if ((cont->sem_id = semget(key, 3, 0666|IPC_CREAT)) < 0)
                {
                    snprintf(cont->err_msg, SHMSBUF_EMSG_SIZE,
                             "[%s:%d] shm_init(): semget() call failed - %s",
                             _FILE_, __LINE__, strerror(errno));
                    shmdt(cont->ptr);
                    return -12;
                }
                if (semctl(cont->sem_id, 2, SETVAL, 1) < 0)
                {
                    snprintf(cont->err_msg, SHMSBUF_EMSG_SIZE,
                             "[%s:%d] shm_init(): error initilizing wake "
                             "semaphore - %s",
                            _FILE_, __LINE__, strerror(errno));
                    return -13;
                }
            }
        }
        else
        {
            snprintf(cont->err_msg, SHMSBUF_EMSG_SIZE,
                     "[%s:%d] shm_init(): semget() call failed - %s",
                     _FILE_, __LINE__, strerror(errno));
            shmdt(cont->ptr);
            return -14;
        }
    }
    else
    {
        if (semctl(cont->sem_id, 2, SETVAL, 1) < 0)
        {
            snprintf(cont->err_msg, SHMSBUF_EMSG_SIZE,
                "[%s:%d] shm_init(): error initilizing wake semaphore - %s",
                _FILE_, __LINE__, strerror(errno));
            return -15;
        }
    }
    strncpy(cont->err_msg, "No errors detected", SHMSBUF_EMSG_SIZE);

    return 0;
}

/* -------------------------------------------------------------------------- */
static int shm_deinit(struct ShmSBuf *cont)
{
    struct shmid_ds shm_desc;
    memset(&shm_desc, 0, sizeof shm_desc);

    if (cont->ptr == NULL)              /* already unused */
    {
        snprintf(cont->err_msg, SHMSBUF_EMSG_SIZE,
                 "[%s:%d] shm_deinit(): shm segment already unused",
                 _FILE_, __LINE__);
        return -2;
    }

    if (shmdt(cont->ptr) < 0)
    {
        snprintf(cont->err_msg, SHMSBUF_EMSG_SIZE,
                 "[%s:%d] shm_deinit(): shmdt() call failed - %s",
                 _FILE_, __LINE__, strerror(errno));
        return -3;
    }

    if (shmctl(cont->shm_id, IPC_STAT, &shm_desc) < 0)
    {
        snprintf(cont->err_msg, SHMSBUF_EMSG_SIZE,
                 "[%s:%d] shm_deinit(): shmctl() call failed - %s",
                 _FILE_, __LINE__, strerror(errno));
        return -4;
    }
    if (shm_desc.shm_nattch == 0)
    {
        if (shmctl(cont->shm_id, IPC_RMID, NULL) < 0)
        {
            snprintf(cont->err_msg, SHMSBUF_EMSG_SIZE,
                     "[%s:%d] shm_deinit(): shmctl() call failed - %s",
                     _FILE_, __LINE__, strerror(errno));
            return -5;
        }
        if (semctl(cont->sem_id, 0, IPC_RMID) < 0)
        {
            snprintf(cont->err_msg, SHMSBUF_EMSG_SIZE,
                     "[%s:%d] shm_deinit(): semctl() call failed - %s",
                     _FILE_, __LINE__, strerror(errno));
            return -6;
        }
    }
    cont->ptr = NULL;
    cont->shm_id = 0;
    cont->sem_id = 0;
    cont->shm_size = 0;
    snprintf(cont->err_msg, SHMSBUF_EMSG_SIZE,
             "Shm segment detached correctly");

    return 0;
}

/* -------------------------------------------------------------------------- */
static void create_new_sbuf(struct ShmSBuf *psbuf,unsigned int nmemb,
                            size_t elsize)
{
    char *ptr = psbuf->ptr;

    memcpy(ptr+OFF_MAGIC, &SHMSBUF_MAGIC, sizeof SHMSBUF_MAGIC);
    memcpy(ptr+OFF_OFF, &SHMSBUF_DATA_OFFSET, sizeof SHMSBUF_DATA_OFFSET);
    memcpy(ptr+OFF_VER, &SHMSBUF_VERSION, sizeof SHMSBUF_VERSION);
    memcpy(ptr+OFF_NMEMB, &nmemb, sizeof nmemb);
    memcpy(ptr+OFF_ELSIZE, &elsize, sizeof elsize);
    psbuf->nmemb = nmemb;
    psbuf->elsize = elsize;
    psbuf->dataoffset = (size_t)SHMSBUF_DATA_OFFSET;
}

/* -------------------------------------------------------------------------- */
static void connect_to_existing_sbuf(struct ShmSBuf *psbuf,unsigned int nmemb,
                                     size_t elsize, int *pret)
{
    char *ptr = psbuf->ptr;
    uint8_t ver;

    memcpy(&ver, ptr+OFF_VER, sizeof ver);
    if (ver != SHMSBUF_VERSION)
    {
        fprintf(stderr, "Unsupported version: %u, using %u anyway...\n",
                ver, SHMSBUF_VERSION);
    }
    memcpy(&psbuf->elsize, ptr+OFF_ELSIZE, sizeof psbuf->elsize);
    memcpy(&psbuf->nmemb, ptr+OFF_NMEMB, sizeof psbuf->nmemb);
    psbuf->dataoffset = 0;
    memcpy(&psbuf->dataoffset, ptr+OFF_OFF, sizeof SHMSBUF_DATA_OFFSET);
    if ((psbuf->elsize != elsize) || (psbuf->nmemb != nmemb))
    {
        *pret = (int)CONN_TO_ANOTHER;
    }
    else
    {
        *pret = (int)CONN_TO_EXISTING;
    }
}

/* -------------------------------------------------------------------------- */
static void connect_to_another_sbuf(struct ShmSBuf *psbuf)
{
    char *ptr = psbuf->ptr;
    uint8_t ver;

    memcpy(&ver, ptr+OFF_VER, sizeof ver);
    if (ver != SHMSBUF_VERSION)
    {
        fprintf(stderr, "Unsupported version: %u, using %u anyway...\n",
                ver, SHMSBUF_VERSION);
    }
    memcpy(&psbuf->elsize, ptr+OFF_ELSIZE, sizeof psbuf->elsize);
    memcpy(&psbuf->nmemb, ptr+OFF_NMEMB, sizeof psbuf->nmemb);
    psbuf->dataoffset = 0;
    memcpy(&psbuf->dataoffset, ptr+OFF_OFF, sizeof SHMSBUF_DATA_OFFSET);
}

/* -------------------------------------------------------------------------- */
int shm_sbuf_init(struct ShmSBuf *psbuf, int proj_id,
                  unsigned int nmemb, size_t elsize)
{
    uint16_t head = UINT16_MAX;
    int ret = 0;

    if (!psbuf)
        return (int)NULL_PTR;
    if (!nmemb || !elsize)
        return (int)BAD_ARGS;
    if (nmemb > UINT_MAX/2)
        return (int)BAD_ARGS;
    if (shm_init(psbuf, proj_id, SHMSBUF_DATA_OFFSET+nmemb*elsize))
        return (int)SHM_ERR;
    /* check if that's existing sbuf or smth else */
    if (semtimedop(psbuf->sem_id, wrlock, 3, &timeout) < 0)
    {
        snprintf(psbuf->err_msg, SHMSBUF_EMSG_SIZE,
                 "[%s:%d] shm_sbuf_init(): error locking write semaphore - %s",
                 _FILE_, __LINE__, strerror(errno));
        return (int)SEM_ERR;
    }

    if (psbuf->shm_size >= sizeof SHMSBUF_MAGIC)
    {
        memcpy(&head, psbuf->ptr, sizeof SHMSBUF_MAGIC);
    }
    if (psbuf->shm_size == (SHMSBUF_DATA_OFFSET+nmemb*elsize))
    { /* that's likely the new segment or needed one */
        if (!(uint16_t)head)
        { /* got zeroes at the beginning */
            create_new_sbuf(psbuf, nmemb, elsize);
            ret = (int)NEW_CREATED;
        }
        else if ((uint16_t)head == SHMSBUF_MAGIC)
        { /* that sbuf's created by another process, it may be the correct one,
          you want to use, or it's just luck that size and magic are correct */
            connect_to_existing_sbuf(psbuf, nmemb, elsize, &ret);
        }
        else
        { /* that's shm segment containing smth you don't need */
            ret = (int)NOT_SHM_SBUF;
        }
    }
    else
    { /* that's likely not shm segment you want to use */
        if ((uint16_t)head == SHMSBUF_MAGIC)
        { /* that's sbuf, but not the one you want to use, check return code */
            connect_to_another_sbuf(psbuf);
            ret = (int)CONN_TO_ANOTHER;
        }
        else
        { /* shm segment doesn't contain sbuf */
            ret = (int)NOT_SHM_SBUF;
        }
    }

    if (semctl(psbuf->sem_id, 1, SETVAL, 0) < 0)
    {
        snprintf(psbuf->err_msg, SHMSBUF_EMSG_SIZE,
            "[%s:%d] shm_sbuf_init(): error unlocking write semaphore - %s",
            _FILE_, __LINE__, strerror(errno));
        return (int)SEM_ERR;
    }
    if (ret < 0)
    {
        shm_deinit(psbuf);
        memset(psbuf, 0, sizeof *psbuf);
    }
    else
    {
        psbuf->last_read = 0;
        psbuf->add = &shm_sbuf_add;
        psbuf->at = &shm_sbuf_at;
        psbuf->deinit = &shm_sbuf_deinit;
        psbuf->last = &shm_sbuf_last;
        psbuf->linearize = &shm_sbuf_linearize;
        psbuf->nread = &shm_sbuf_nread;
        strncpy(psbuf->err_msg, shm_sbuf_error(ret), SHMSBUF_EMSG_SIZE);
    }

    return ret;
}

/* -------------------------------------------------------------------------- */
int shm_sbuf_deinit(struct ShmSBuf *psbuf)
{
    if (!psbuf)
        return (int)NULL_PTR;
    if (shm_deinit(psbuf))
        return (int)SHM_ERR;
    memset(psbuf, 0, sizeof *psbuf);
    return (int)NOERR;
}

/* -------------------------------------------------------------------------- */
int shm_sbuf_at(struct ShmSBuf *psbuf, int idx, void *dst)
{
    int ret = (int)NOERR;
    unsigned int total;
    const char *ptr = psbuf->ptr;

    if (!psbuf || !dst)
        return (int)NULL_PTR;
    if ((idx > (int)psbuf->nmemb-1) || (idx < -(int)psbuf->nmemb))
        return (int)BAD_ARGS;
    if (semtimedop(psbuf->sem_id, rdlock, 2, &timeout) < 0)
    {
        snprintf(psbuf->err_msg, SHMSBUF_EMSG_SIZE,
                 "[%s:%d] shm_sbuf_at(): error locking read semaphore - %s",
                 _FILE_, __LINE__, strerror(errno));
        return (int)SEM_ERR;
    }
    memcpy(&total, ptr+OFF_CNT, sizeof total);
    if (idx >= 0)
    {
        if (total < psbuf->nmemb)
        {
            if (idx > (int)total-1)
            {
                ret = (int)BAD_ARGS;
                snprintf(psbuf->err_msg, SHMSBUF_EMSG_SIZE,
                    "[%s:%d] shm_sbuf_at(): idx out of range, idx = %d > %d",
                    _FILE_, __LINE__, idx, (int)total-1);
            }
            else
            {
                memcpy(dst, ptr+psbuf->dataoffset+idx*psbuf->elsize,
                       psbuf->elsize);
                if ((unsigned int)(idx+1) > psbuf->last_read)
                {
                    psbuf->last_read = idx+1;
                }
            }
        }
        else
        {
            unsigned int cur_idx;

            memcpy(&cur_idx, ptr+OFF_IDX, sizeof cur_idx);
            memcpy(dst, ptr + psbuf->dataoffset +
                   ((cur_idx + idx) % psbuf->nmemb) * psbuf->elsize,
                   psbuf->elsize);
            if ((total - psbuf->nmemb + idx + 1) > psbuf->last_read)
            {
                psbuf->last_read = total - psbuf->nmemb + idx + 1;
            }
        }
    }
    else
    {
        if (total < psbuf->nmemb)
        {
            if (-idx > (int)total)
            {
                ret = (int)BAD_ARGS;
                snprintf(psbuf->err_msg, SHMSBUF_EMSG_SIZE,
                    "[%s:%d] shm_sbuf_at(): idx out of range, -idx = %d > %d",
                    _FILE_, __LINE__, -idx, (int)total);
            }
            else
            {
                memcpy(dst, ptr+psbuf->dataoffset+(total+idx)*psbuf->elsize,
                       psbuf->elsize);
                if ((total + idx + 1) > psbuf->last_read)
                {
                    psbuf->last_read = total + idx + 1;
                }
            }
        }
        else
        {
            unsigned int cur_idx;

            memcpy(&cur_idx, ptr+OFF_IDX, sizeof cur_idx);
            memcpy(dst, ptr+psbuf->dataoffset+
                   ((cur_idx+psbuf->nmemb+idx)%psbuf->nmemb)*psbuf->elsize,
                   psbuf->elsize);
            if ((total + idx + 1) > psbuf->last_read)
            {
                psbuf->last_read = total + idx + 1;
            }
        }
    }
    if (semop(psbuf->sem_id, rdunlock, 1) < 0)
    {
        snprintf(psbuf->err_msg, SHMSBUF_EMSG_SIZE,
                 "[%s:%d] shm_sbuf_at(): error unlocking read semaphore - %s",
                 _FILE_, __LINE__, strerror(errno));
        return (int)SEM_ERR;
    }
    return ret;
}

/* -------------------------------------------------------------------------- */
int shm_sbuf_last(struct ShmSBuf *psbuf, void *dst)
{
    unsigned int total;
    unsigned int idx;
    int ret = (int)NOERR;
    const char *ptr = psbuf->ptr;

    if (!psbuf || !dst)
        return (int)NULL_PTR;
    if (semtimedop(psbuf->sem_id, rdlock, 2, &timeout) < 0)
    {
        snprintf(psbuf->err_msg, SHMSBUF_EMSG_SIZE,
                 "[%s:%d] shm_sbuf_last(): error locking read semaphore - %s",
                 _FILE_, __LINE__, strerror(errno));
        return (int)SEM_ERR;
    }
    memcpy(&total, ptr+OFF_CNT, sizeof total);
    memcpy(&idx, ptr+OFF_IDX, sizeof idx);
    if (total < psbuf->nmemb)
    {
        if (idx)
        {
            memcpy(dst, ptr+psbuf->dataoffset+(idx-1)*psbuf->elsize,
                   psbuf->elsize);
            psbuf->last_read = total;
        }
        else
        {
            ret = (int)SOME_ERR;
            strncpy(psbuf->err_msg, "shm_sbuf_last(): buffer is empty",
                    SHMSBUF_EMSG_SIZE);
        }
    }
    else
    {
        if (idx)
        {
            memcpy(dst, ptr+psbuf->dataoffset+(idx-1)*psbuf->elsize,
                   psbuf->elsize);
        }
        else
        {
            memcpy(dst, ptr+psbuf->dataoffset+(psbuf->nmemb-1)*psbuf->elsize,
                   psbuf->elsize);
        }
        psbuf->last_read = total;
    }
    if (semop(psbuf->sem_id, rdunlock, 1) < 0)
    {
        snprintf(psbuf->err_msg, SHMSBUF_EMSG_SIZE,
                 "[%s:%d] shm_sbuf_last(): error unlocking read semaphore - %s",
                 _FILE_, __LINE__, strerror(errno));
        return (int)SEM_ERR;
    }
    return ret;
}

/* -------------------------------------------------------------------------- */
int shm_sbuf_nread(struct ShmSBuf *psbuf, void *dst, struct timespec *to)
{
    unsigned int total;
    unsigned int idx;
    int ret = (int)NOERR;
    unsigned int n_new = 0;
    const char *ptr = psbuf->ptr;

    if (!psbuf || !dst)
        return (int)NULL_PTR;
    if (semtimedop(psbuf->sem_id, rdlock, 2, &timeout) < 0)
    {
        snprintf(psbuf->err_msg, SHMSBUF_EMSG_SIZE,
                 "[%s:%d] shm_sbuf_nread(): error locking read semaphore - %s",
                 _FILE_, __LINE__, strerror(errno));
        return (int)SEM_ERR;
    }
    memcpy(&total, ptr+OFF_CNT, sizeof total);
    if ((n_new = total - psbuf->last_read) > 0)
    {
        if (n_new > psbuf->nmemb)
            n_new = psbuf->nmemb;
        ret = n_new;
        memcpy(&idx, ptr+OFF_IDX, sizeof idx);
        if (total < psbuf->nmemb)
        {
            if (idx)
            {
                memcpy(dst, ptr+psbuf->dataoffset+(total-n_new)*psbuf->elsize,
                       n_new*psbuf->elsize);
                psbuf->last_read = total;
            }
            else
            {
                ret = (int)SOME_ERR;
                strncpy(psbuf->err_msg, "shm_sbuf_last(): buffer is empty",
                        SHMSBUF_EMSG_SIZE);
            }
        }
        else
        {
            int tmp = n_new - idx;
            if (tmp >= 0)
            {
                memcpy(dst, ptr + psbuf->dataoffset +
                       (psbuf->nmemb-tmp)*psbuf->elsize, tmp*psbuf->elsize);
                memcpy((char*)dst + tmp*psbuf->elsize, ptr + psbuf->dataoffset,
                       idx*psbuf->elsize);
            }
            else
            {
                memcpy(dst, ptr + psbuf->dataoffset + (idx-n_new)*psbuf->elsize,
                       n_new*psbuf->elsize);
            }
            psbuf->last_read = total;
        }
    }
    if (semop(psbuf->sem_id, rdunlock, 1) < 0)
    {
        snprintf(psbuf->err_msg, SHMSBUF_EMSG_SIZE,
                 "[%s:%d] shm_sbuf_raw(): error unlocking read semaphore - %s",
                 _FILE_, __LINE__, strerror(errno));
        return (int)SEM_ERR;
    }
    if (ret != (int)NOERR) /* new data was available immediately or error */
        return ret;
    /* now wait for new data */
    if (semtimedop(psbuf->sem_id, wakewait, 1, to) < 0)
    {
        if (errno == EAGAIN)
        {
            ret = (int)SEM_TIMEOUT;
        }
        else
        {
            ret = (int)SEM_ERR;
        }
        snprintf(psbuf->err_msg, SHMSBUF_EMSG_SIZE,
                 "[%s:%d] shm_sbuf_nread(): error waiting wake semaphore - %s",
                 _FILE_, __LINE__, strerror(errno));
        return ret;
    }
    /* wait ends here, read new data */
    if (semtimedop(psbuf->sem_id, rdlock, 2, &timeout) < 0)
    {
        snprintf(psbuf->err_msg, SHMSBUF_EMSG_SIZE,
                 "[%s:%d] shm_sbuf_nread(): error locking read semaphore - %s",
                 _FILE_, __LINE__, strerror(errno));
        return (int)SEM_ERR;
    }
    memcpy(&total, ptr+OFF_CNT, sizeof total);
    if ((n_new = total - psbuf->last_read) > 0)
    {
        if (n_new > psbuf->nmemb)
            n_new = psbuf->nmemb;
        ret = n_new;
        memcpy(&idx, ptr+OFF_IDX, sizeof idx);
        if (total < psbuf->nmemb)
        {
            if (idx)
            {
                memcpy(dst, ptr+psbuf->dataoffset+(total-n_new)*psbuf->elsize,
                       n_new*psbuf->elsize);
                psbuf->last_read = total;
            }
            else
            {
                ret = (int)SOME_ERR;
                strncpy(psbuf->err_msg, "shm_sbuf_last(): buffer is empty",
                        SHMSBUF_EMSG_SIZE);
            }
        }
        else
        {
            int tmp = n_new - idx;
            if (tmp >= 0)
            {
                memcpy(dst, ptr + psbuf->dataoffset +
                       (psbuf->nmemb-tmp)*psbuf->elsize, tmp*psbuf->elsize);
                memcpy((char*)dst + tmp*psbuf->elsize, ptr + psbuf->dataoffset,
                       idx*psbuf->elsize);
            }
            else
            {
                memcpy(dst, ptr + psbuf->dataoffset + (idx-n_new)*psbuf->elsize,
                       n_new*psbuf->elsize);
            }
            psbuf->last_read = total;
        }
    }
    else
        ret = (int)SOME_ERR;
    if (semop(psbuf->sem_id, rdunlock, 1) < 0)
    {
        snprintf(psbuf->err_msg, SHMSBUF_EMSG_SIZE,
                 "[%s:%d] shm_sbuf_raw(): error unlocking read semaphore - %s",
                 _FILE_, __LINE__, strerror(errno));
        return (int)SEM_ERR;
    }
    return ret;
}

/* -------------------------------------------------------------------------- */
int shm_sbuf_raw(struct ShmSBuf *psbuf, void *dst)
{
    if (!psbuf || !dst)
        return (int)NULL_PTR;
    if (semtimedop(psbuf->sem_id, rdlock, 2, &timeout) < 0)
    {
        snprintf(psbuf->err_msg, SHMSBUF_EMSG_SIZE,
                 "[%s:%d] shm_sbuf_raw(): error locking read semaphore - %s",
                 _FILE_, __LINE__, strerror(errno));
        return (int)SEM_ERR;
    }
    memcpy(dst, psbuf->ptr, psbuf->shm_size);
    if (semop(psbuf->sem_id, rdunlock, 1) < 0)
    {
        snprintf(psbuf->err_msg, SHMSBUF_EMSG_SIZE,
                 "[%s:%d] shm_sbuf_raw(): error unlocking read semaphore - %s",
                 _FILE_, __LINE__, strerror(errno));
        return (int)SEM_ERR;
    }
    return (int)NOERR;
}

/* -------------------------------------------------------------------------- */
int shm_sbuf_data(struct ShmSBuf *psbuf, void *dst)
{
    if (!psbuf || !dst)
        return (int)NULL_PTR;
    if (semtimedop(psbuf->sem_id, rdlock, 2, &timeout) < 0)
    {
        snprintf(psbuf->err_msg, SHMSBUF_EMSG_SIZE,
                 "[%s:%d] shm_sbuf_data(): error locking read semaphore - %s",
                 _FILE_, __LINE__, strerror(errno));
        return (int)SEM_ERR;
    }
    memcpy(dst, (char*)psbuf->ptr+psbuf->dataoffset,
           psbuf->elsize*psbuf->nmemb);
    if (semop(psbuf->sem_id, rdunlock, 1) < 0)
    {
        snprintf(psbuf->err_msg, SHMSBUF_EMSG_SIZE,
                 "[%s:%d] shm_sbuf_data(): error unlocking read semaphore - %s",
                 _FILE_, __LINE__, strerror(errno));
        return (int)SEM_ERR;
    }
    return (int)NOERR;
}

/* -------------------------------------------------------------------------- */
int shm_sbuf_linearize(struct ShmSBuf *psbuf, void *dst)
{
    unsigned int total;
    const char *ptr = psbuf->ptr;

    if (!psbuf || !dst)
        return (int)NULL_PTR;
    if (semtimedop(psbuf->sem_id, rdlock, 2, &timeout) < 0)
    {
        snprintf(psbuf->err_msg, SHMSBUF_EMSG_SIZE,
            "[%s:%d] shm_sbuf_linearize(): error locking read semaphore - %s",
            _FILE_, __LINE__, strerror(errno));
        return (int)SEM_ERR;
    }
    memcpy(&total, ptr+OFF_CNT, sizeof total);
    if (total < psbuf->nmemb)
    {
        memcpy(dst, ptr+psbuf->dataoffset, psbuf->elsize*total);
    }
    else
    {
        unsigned int idx;

        memcpy(&idx, ptr+OFF_IDX, sizeof idx);
        memcpy(dst, ptr+psbuf->dataoffset+idx*psbuf->elsize,
               (psbuf->nmemb-idx)*psbuf->elsize);
        if (idx)
        {
            memcpy((char*)dst+(psbuf->nmemb-idx)*psbuf->elsize,
                   ptr+psbuf->dataoffset, idx*psbuf->elsize);
        }
    }
    psbuf->last_read = total;
    if (semop(psbuf->sem_id, rdunlock, 1) < 0)
    {
        snprintf(psbuf->err_msg, SHMSBUF_EMSG_SIZE,
            "[%s:%d] shm_sbuf_linearize(): error unlocking read semaphore - %s",
            _FILE_, __LINE__, strerror(errno));
        return (int)SEM_ERR;
    }
    return (int)NOERR;
}

/* -------------------------------------------------------------------------- */
int shm_sbuf_add(struct ShmSBuf *psbuf, const void *src, unsigned int nmemb)
{
    size_t startoff = 0;
    unsigned int quantity;
    unsigned int total;
    unsigned int idx;
    unsigned int tmp;
    char *ptr = psbuf->ptr;

    if (!psbuf || !src)
        return (int)NULL_PTR;
    if (!nmemb)
        return (int)BAD_ARGS;
    if (nmemb > psbuf->nmemb)
    {
        quantity = psbuf->nmemb;
        startoff = (size_t)((nmemb - quantity)*psbuf->elsize);
    }
    else
    {
        quantity = nmemb;
    }
    if (semtimedop(psbuf->sem_id, wrlock, 3, &timeout) < 0)
    {
        snprintf(psbuf->err_msg, SHMSBUF_EMSG_SIZE,
                 "[%s:%d] shm_sbuf_add(): error locking write semaphore - %s",
                 _FILE_, __LINE__, strerror(errno));
        return (int)SEM_ERR;
    }
    memcpy(&total, ptr+OFF_CNT, sizeof total);
    memcpy(&idx, ptr+OFF_IDX, sizeof idx);
    if ((tmp = psbuf->nmemb-idx) < quantity)
    {
        memcpy(ptr+psbuf->dataoffset+idx*psbuf->elsize,
               (const char*)src+startoff,
               tmp*psbuf->elsize);
        memcpy(ptr+psbuf->dataoffset,
               (const char*)src+startoff+tmp*psbuf->elsize,
               (quantity-tmp)*psbuf->elsize);
        idx = quantity-tmp;
    }
    else
    {
        memcpy(ptr+psbuf->dataoffset+idx*psbuf->elsize,
               (const char*)src+startoff,
               quantity*psbuf->elsize);
        idx += quantity;
        idx %= psbuf->nmemb;
    }
    total += quantity;
    memcpy(ptr+OFF_CNT, &total, sizeof total);
    memcpy(ptr+OFF_IDX, &idx, sizeof idx);
    /* now wake processes waiting for new data */
    if (semctl(psbuf->sem_id, 2, SETVAL, 0) < 0)
    {
        snprintf(psbuf->err_msg, SHMSBUF_EMSG_SIZE,
                 "[%s:%d] shm_sbuf_add(): error unlocking wake semaphore - %s",
                 _FILE_, __LINE__, strerror(errno));
        return (int)SEM_ERR;
    }
    if (semop(psbuf->sem_id, wakelock, 1) < 0)
    {
        snprintf(psbuf->err_msg, SHMSBUF_EMSG_SIZE,
                 "[%s:%d] shm_sbuf_add(): error locking wake semaphore - %s",
                 _FILE_, __LINE__, strerror(errno));
        return (int)SEM_ERR;
    }
    /* wake ends here */
    if (semctl(psbuf->sem_id, 1, SETVAL, 0) < 0)
    {
        snprintf(psbuf->err_msg, SHMSBUF_EMSG_SIZE,
                 "[%s:%d] shm_sbuf_add(): error unlocking write semaphore - %s",
                 _FILE_, __LINE__, strerror(errno));
        return (int)SEM_ERR;
    }
    return (int)NOERR;
}

/* -------------------------------------------------------------------------- */
static void shm_seg_info(struct ShmSBuf *cont, FILE *stream)
{
    char buf[128];
    unsigned int i;
    union semun
    {
        int               val;
        struct semid_ds   *buf;
        unsigned short    *array;
        struct seminfo    *__buf;
    } sem_un;
    struct shmid_ds shm_desc;
    struct semid_ds sem_desc;

    if (cont == NULL)
        return;
    if (!stream)
        stream = stdout;

    memset(&sem_un, 0, sizeof sem_un);
    memset(&shm_desc, 0, sizeof shm_desc);
    memset(&sem_desc, 0, sizeof sem_desc);
    sem_un.buf = &sem_desc;

    shmctl(cont->shm_id, IPC_STAT, &shm_desc);
    semctl(cont->sem_id, 0, IPC_STAT, sem_un);

    fprintf(stream,"\n\
-------------------------------------------------------------------------\n\
           Info for segment:\n\
shm segment id         %d\n\
semaphore id           %d\n\
size of segment        %u\n\
pointer of segment     %p\n\
error message          %s\n",
            cont->shm_id,
            cont->sem_id,
            (unsigned int)cont->shm_size,
            cont->ptr,
            cont->err_msg);
    fprintf(stream,"\n\
           Stat of shm:\n\
Ownership and permissions:\n\
    Key supplied to shmget(2)    %d\n\
    Effective UID of owner       %d\n\
    Effective GID of owner       %d\n\
    Effective UID of creator     %d\n\
    Effective GID of creator     %d\n\
    Permissions + SHM_DEST\n\
      and SHM_LOCKED flags       %#o\n\
Size of segment (bytes)          %d\n",
            shm_desc.shm_perm.__key,
            (int)shm_desc.shm_perm.uid,
            (int)shm_desc.shm_perm.gid,
            (int)shm_desc.shm_perm.cuid,
            (int)shm_desc.shm_perm.cgid,
            shm_desc.shm_perm.mode,
            (int)shm_desc.shm_segsz);
    strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S",
             localtime(&shm_desc.shm_atime));
    fprintf(stream, "\
Last attach time                 %s\n", buf);
    strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S",
             localtime(&shm_desc.shm_dtime));
    fprintf(stream, "\
Last detach time                 %s\n", buf);
    strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S",
             localtime(&shm_desc.shm_ctime));
    fprintf(stream, "\
Last change time                 %s\n", buf);
    fprintf(stream, "\
PID of creator                   %d\n\
PID of last shmat(2)/shmdt(2)    %d\n\
No. of current attaches          %d\n",
            shm_desc.shm_cpid,
            shm_desc.shm_lpid,
            (int)shm_desc.shm_nattch);

    fprintf(stream, "\n\
           Stat of sem:\n\
Ownership and permissions:\n\
    Key supplied to semget(2)    %d\n\
    Effective UID of owner       %d\n\
    Effective GID of owner       %d\n\
    Effective UID of creator     %d\n\
    Effective GID of creator     %d\n\
    Permissions                  %#o\n\
Sequence number                  %#o\n",
            sem_desc.sem_perm.__key,
            (int)sem_desc.sem_perm.uid,
            (int)sem_desc.sem_perm.gid,
            (int)sem_desc.sem_perm.cuid,
            (int)sem_desc.sem_perm.cgid,
            sem_desc.sem_perm.mode,
            sem_desc.sem_perm.__seq);
    strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S",
             localtime(&sem_desc.sem_otime));
    fprintf(stream, "\
Last semop time                  %s\n", buf);
    strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S",
             localtime(&sem_desc.sem_ctime));
    fprintf(stream, "\
Last change time                 %s\n", buf);
    fprintf(stream, "\
No. of semaphores in set         %lu\n", sem_desc.sem_nsems);
    for (i=0; i<sem_desc.sem_nsems; ++i)
    {
        fprintf(stream, "\
Current value of %3u sem:        %d\n", i, semctl(cont->sem_id, i, GETVAL));
    }
}

/* -------------------------------------------------------------------------- */
int shm_sbuf_info(struct ShmSBuf *psbuf, enum shmsbuf_info_type type, FILE *fd)
{
    struct ShmSBuf_header head;

    if (!psbuf || !fd)
        return (int)NULL_PTR;
    fprintf(fd, "Using ShmSBuf library version %u\n", SHMSBUF_VERSION);
    switch (type)
    {
        case SIT_ALL:
            fprintf(fd, "Verbosity - all\n");
            break;
        case SIT_BUF:
            fprintf(fd, "Verbosity - buffer info\n");
            break;
        case SIT_SHMCI:
            fprintf(fd, "Verbosity - shmci info\n");
            break;
        default:
            fprintf(fd, "Illegal type (hex): %X\n", type);
    }
    if ((type == SIT_ALL) || (type == SIT_SHMCI))
    {
        shm_seg_info(psbuf, fd);
    }
    if ((type == SIT_ALL) || (type == SIT_BUF))
    {
        shm_sbuf_header(psbuf, &head);
        fprintf(fd,"\n\
           Shm slide buffer header:\n\
Magic number (hex)          %02X\n\
Data offset                 %u byte\n\
Library version             %u\n\
Number of elements          %u\n\
Size of element             %u byte\n\
Total elements written      %u\n\
Index pointer               %u\n\n",
                head.magic, head.dataoffset, head.ver, head.nmemb,
                (unsigned int)head.elsize, head.cnt, head.idx);
    }
    return (int)NOERR;
}

/* -------------------------------------------------------------------------- */
int shm_sbuf_header(struct ShmSBuf *psbuf, struct ShmSBuf_header *head)
{
    char *tmp;

    if (!psbuf || !head)
        return (int)NULL_PTR;
    tmp = (char*)malloc(psbuf->dataoffset);
    if (!tmp)
    {
        fprintf(stderr, "Memory allocation failed\n");
        return (int)SOME_ERR;
    }
    if (semtimedop(psbuf->sem_id, rdlock, 2, &timeout) < 0)
    {
        snprintf(psbuf->err_msg, SHMSBUF_EMSG_SIZE,
                 "[%s:%d] shm_sbuf_header(): error locking read semaphore - %s",
                 _FILE_, __LINE__, strerror(errno));
        free(tmp);
        return (int)SEM_ERR;
    }
    memcpy(tmp, psbuf->ptr, psbuf->dataoffset);
    if (semop(psbuf->sem_id, rdunlock, 1) < 0)
    {
        snprintf(psbuf->err_msg, SHMSBUF_EMSG_SIZE,
            "[%s:%d] shm_sbuf_header(): error unlocking read semaphore - %s",
            _FILE_, __LINE__, strerror(errno));
        free(tmp);
        return (int)SEM_ERR;
    }
    memcpy(&head->magic, tmp+OFF_MAGIC, sizeof head->magic);
    memcpy(&head->dataoffset, tmp+OFF_OFF, sizeof head->dataoffset);
    memcpy(&head->ver, tmp+OFF_VER, sizeof head->ver);
    memcpy(&head->nmemb, tmp+OFF_NMEMB, sizeof head->nmemb);
    memcpy(&head->elsize, tmp+OFF_ELSIZE, sizeof head->elsize);
    memcpy(&head->cnt, tmp+OFF_CNT, sizeof head->cnt);
    memcpy(&head->idx, tmp+OFF_IDX, sizeof head->idx);
    free(tmp);
    return (int)NOERR;
}

/* -------------------------------------------------------------------------- */
const char* shm_sbuf_error(int code)
{
    switch (code)
    {
        case (int)NEW_CREATED:
            return "New shm slide buffer was created";
        case (int)CONN_TO_ANOTHER:
            return "Connected to some shm buffer with another parameters";
        case (int)CONN_TO_EXISTING:
            return "Connected to existing shm buffer with required parameters";
        case (int)NOERR:
            return "No errors";
        case (int)NULL_PTR:
            return "NULL pointer received";
        case (int)BAD_ARGS:
            return "Illegal argument received";
        case (int)SHM_ERR:
            return "Shared memory operation error";
        case (int)SEM_ERR:
            return "Semaphore operation error";
        case (int)NOT_SHM_SBUF:
            return "Existing shm segment doesn't contain shm buffer";
        case (int)SOME_ERR:
            return "Some error occured";
        case (int)SEM_TIMEOUT:
            return "Semaphore operation timeout";
        default:
            return "Code has no description";
    }
}
