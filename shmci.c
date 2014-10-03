#include "shmci.h"

#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/sem.h>
#include <sys/shm.h>

#ifndef _FILE_
#define _FILE_ (strrchr(__FILE__,'/') ? strrchr(__FILE__,'/')+1 : __FILE__ )
#endif

static struct timespec timeout={.tv_sec=3,.tv_nsec=0};

static struct sembuf lock[] =
{
  {0, 0, 0},
  {0, 1, SEM_UNDO}
};

static const char std_err_msg[] = "ShmLink or ShmBuf is NULL pointer";

/* ========================================================================= */
static inline int shm_write_generic(const void *dptr, unsigned int offset,
                                    unsigned int size, ShmLink *cont)
{
  if (dptr == NULL)
  {
    (void)snprintf(cont->err_msg, SHMCI_EMSG_SIZE,
                   "[%s:%d] Received NULL data pointer",
                   _FILE_, __LINE__);
    return -1;
  }
  if (cont->ptr == NULL)
  {
    (void)snprintf(cont->err_msg, SHMCI_EMSG_SIZE,
                   "[%s:%d] Shm segment is unused",
                   _FILE_, __LINE__);
    return -2;
  }
  if ((offset + size) > cont->shm_size)
  {
    (void)snprintf(cont->err_msg, SHMCI_EMSG_SIZE,
                   "[%s:%d] To much data to write in segment",
                   _FILE_, __LINE__);
    return -3;
  }

  memcpy((char*)cont->ptr + offset, dptr, size);

  return 0;
}

/* ========================================================================= */
static inline int shm_read_generic(void *dptr, unsigned int offset,
                                   unsigned int size, ShmLink *cont)
{
  if (dptr == NULL)
  {
    (void)snprintf(cont->err_msg, SHMCI_EMSG_SIZE,
                   "[%s:%d] Received NULL data pointer",
                   _FILE_, __LINE__);
    return -1;
  }
  if (cont->ptr == NULL)
  {
    (void)snprintf(cont->err_msg, SHMCI_EMSG_SIZE,
                   "[%s:%d] Shm segment is unused",
                   _FILE_, __LINE__);
    return -2;
  }
  if ((offset + size) > cont->shm_size)
  {
    (void)snprintf(cont->err_msg, SHMCI_EMSG_SIZE,
                   "[%s:%d] To much data to read in segment",
                   _FILE_, __LINE__);
    return -3;
  }

  memcpy(dptr, (char*)cont->ptr + offset, size);

  return 0;
}

/* ========================================================================= */
int shm_disconnect(ShmLink *cont)
{
  struct shmid_ds shm_desc;
  memset(&shm_desc, 0, sizeof shm_desc);

  if (cont == NULL) return -1;
  if (cont->ptr == NULL)              /* already unused */
  {
    (void)snprintf(cont->err_msg, SHMCI_EMSG_SIZE,
                   "[%s:%d] Shm segment already unused",
                   _FILE_, __LINE__);
    return -2;
  }

  if (shmdt(cont->ptr) < 0)
  {
    (void)snprintf(cont->err_msg, SHMCI_EMSG_SIZE,
                   "[%s:%d] Error detaching segment. %s",
                   _FILE_, __LINE__, strerror(errno));
    return -3;
  }

  if (shmctl(cont->shm_id, IPC_STAT, &shm_desc) < 0)
  {
    (void)snprintf(cont->err_msg, SHMCI_EMSG_SIZE,
                   "[%s:%d] Error getting segment stat. %s",
                   _FILE_, __LINE__, strerror(errno));
    return -4;
  }
  if (shm_desc.shm_nattch == 0)
  {
    if (shmctl(cont->shm_id, IPC_RMID, NULL) < 0)
    {
      (void)snprintf(cont->err_msg, SHMCI_EMSG_SIZE,
                     "[%s:%d] Error deleting segment. %s",
                     _FILE_, __LINE__, strerror(errno));
      return -5;
    }
    if (semctl(cont->sem_id, 0, IPC_RMID) < 0)
    {
      (void)snprintf(cont->err_msg, SHMCI_EMSG_SIZE,
                     "[%s:%d] Error deleting semaphore. %s",
                     _FILE_, __LINE__, strerror(errno));
      return -6;
    }
  }
  cont->ptr = NULL;
  cont->shm_id = 0;
  cont->sem_id = 0;
  cont->shm_size = 0;
  (void)snprintf(cont->err_msg, SHMCI_EMSG_SIZE,
                 "Shm segment disconnected correctly");

  return 0;
}

/* ========================================================================= */
int shm_connect(ShmLink *cont, const char* key_path, const int key_int,
                unsigned int size)
{
  int semflag = 0;
  key_t key;
  struct shmid_ds shm_desc;
  memset(&shm_desc, 0, sizeof shm_desc);

  if (cont == NULL || key_path == NULL) return -1;
  if ((key=ftok(key_path,key_int)) < 0) return -2;
  /* Try to create shm segment */
  if ((cont->shm_id = shmget(key, (size_t)size, 0666|IPC_CREAT|IPC_EXCL)) < 0)
  {
    if (errno == EEXIST)   /* Shm segment for this key exists already */
    {
      if ((cont->shm_id = shmget(key, 1, 0666)) < 0) /*Get ID of existing seg*/
      {
        (void)snprintf(cont->err_msg, SHMCI_EMSG_SIZE,
                       "[%s:%d] Error in shmget() for segment. %s",
                       _FILE_, __LINE__, strerror(errno));
        return -4;
      }
      if (shmctl(cont->shm_id, IPC_STAT, &shm_desc) < 0)
      {
        (void)snprintf(cont->err_msg, SHMCI_EMSG_SIZE,
                       "[%s:%d] Error getting segment stat. %s",
                       _FILE_, __LINE__, strerror(errno));
        return -5;
      }
      if (shm_desc.shm_nattch == 0) /* Segment unused - delete it and create anew */
      {
        semflag = 1;        /* Flag - recreate semaphore */
        if (shmctl(cont->shm_id, IPC_RMID, NULL) < 0)
        {
          (void)snprintf(cont->err_msg, SHMCI_EMSG_SIZE,
                         "[%s:%d] Error deleting old segment. %s",
                         _FILE_, __LINE__, strerror(errno));
          return -6;
        }
        if ((cont->shm_id = shmget(key, (size_t)size, 0666|IPC_CREAT)) < 0)
        {
          (void)snprintf(cont->err_msg, SHMCI_EMSG_SIZE,
                         "[%s:%d] Error in shmget() for segment. %s",
                         _FILE_, __LINE__, strerror(errno));
          return -7;
        }
        cont->shm_size = size;
      }
      else /* Segment is already in use */
      {
        cont->shm_size = (unsigned int)shm_desc.shm_segsz;
      }
    }
    else
    {
      (void)snprintf(cont->err_msg, SHMCI_EMSG_SIZE,
                    "[%s:%d] Unexpected error in shmget() for segment. %s",
                    _FILE_, __LINE__, strerror(errno));
      return -8;
    }
  }
  else /* Segment created */
  {
    cont->shm_size=size;
  }
  /* Attach new shm segment to address space */
  if ((cont->ptr = shmat(cont->shm_id, NULL, 0)) == (void*)-1)
  {
    (void)snprintf(cont->err_msg, SHMCI_EMSG_SIZE,
                   "[%s:%d] Error attaching segment. %s",
                   _FILE_, __LINE__, strerror(errno));
    return -9;
  }
  /* Try to create semaphore */
  if ((cont->sem_id = semget(key, 1, 0666|IPC_CREAT|IPC_EXCL)) < 0)
  {
    if (errno == EEXIST) /* Semaphore set for this key exists already */
    {
      if ((cont->sem_id = semget(key, 1, 0666)) < 0) /*Get ID of existing sem*/
      {
        (void)snprintf(cont->err_msg, SHMCI_EMSG_SIZE,
                       "[%s:%d] Error in semget() for semaphore. %s",
                       _FILE_, __LINE__, strerror(errno));
        return -10;
      }
      if (semflag != 0)
      {
        if (semctl(cont->sem_id, 0, IPC_RMID) < 0)
        {
          (void)snprintf(cont->err_msg, SHMCI_EMSG_SIZE,
                         "[%s:%d] Error deleting semaphore. %s",
                         _FILE_, __LINE__, strerror(errno));
          return -11;
        }
        if ((cont->sem_id = semget(key, 1, 0666|IPC_CREAT)) < 0)
        {
          (void)snprintf(cont->err_msg, SHMCI_EMSG_SIZE,
                         "[%s:%d] Error in semget() for semaphore. %s",
                         _FILE_, __LINE__, strerror(errno));
          return -12;
        }
      }
    }
    else
    {
      (void)snprintf(cont->err_msg, SHMCI_EMSG_SIZE,
                     "[%s:%d] Unexpected error in semget() for semaphore. %s",
                     _FILE_, __LINE__, strerror(errno));
      return -13;
    }
  }
  (void)strncpy(cont->err_msg, "No errors detected", SHMCI_EMSG_SIZE);

  return 0;
}

/* ========================================================================= */
int shm_clean(ShmLink *cont)
{
  if (cont == NULL) return -1;
  if (semtimedop(cont->sem_id, lock, 2, &timeout) < 0)
  {
    (void)snprintf(cont->err_msg, SHMCI_EMSG_SIZE,
                   "[%s:%d] Error locking semaphore. %s",
                   _FILE_, __LINE__, strerror(errno));
    return -2;
  }

  memset(cont->ptr,0,cont->shm_size);

  if (semctl(cont->sem_id, 0, SETVAL, 0) < 0)
  {
    (void)snprintf(cont->err_msg, SHMCI_EMSG_SIZE,
                   "[%s:%d] Error unlocking semaphore. %s",
                   _FILE_, __LINE__, strerror(errno));
     return -3;
  }
  return 0;
}

/* ========================================================================= */
int shm_write(ShmLink *cont, const void *dptr, unsigned int offset,
              unsigned int size)
{
  int res;

  if (cont == NULL) return -1;
  if (semtimedop(cont->sem_id, lock, 2, &timeout) < 0)
  {
    (void)snprintf(cont->err_msg, SHMCI_EMSG_SIZE,
                   "[%s:%d] Error locking semaphore. %s",
                   _FILE_, __LINE__, strerror(errno));
    return -2;
  }

  res = shm_write_generic(dptr, offset, size, cont);

  if (semctl(cont->sem_id, 0, SETVAL, 0) < 0)
  {
    (void)snprintf(cont->err_msg, SHMCI_EMSG_SIZE,
                   "[%s:%d] Error unlocking semaphore. %s",
                   _FILE_, __LINE__, strerror(errno));
    return -3;
  }
  if (res < 0) return -4; /* error description has been already written */
  else return 0;
}

/* ========================================================================= */
int shm_read(ShmLink *cont, void *dptr, unsigned int offset,
             unsigned int size)
{
  int res;

  if (cont == NULL) return -1;
  if (semtimedop(cont->sem_id, lock, 2, &timeout) < 0)
  {
    (void)snprintf(cont->err_msg, SHMCI_EMSG_SIZE,
                   "[%s:%d] Error locking semaphore. %s",
                   _FILE_, __LINE__, strerror(errno));
    return -3;
  }

  res = shm_read_generic(dptr, offset, size, cont);

  if (semctl(cont->sem_id, 0, SETVAL, 0) < 0)
  {
    (void)snprintf(cont->err_msg, SHMCI_EMSG_SIZE,
                   "[%s:%d] Error unlocking semaphore. %s",
                   _FILE_, __LINE__, strerror(errno));
    return -4;
  }
  if (res < 0) return -5; /* error description has been already written */
  else return 0;
}

/* ========================================================================= */
const char* shm_strerror(ShmLink *cont)
{
  if (cont == NULL) return std_err_msg;
  else return cont->err_msg;
}

/* ========================================================================= */
void shm_printinfo(ShmLink *cont, FILE *stream)
{
  char buf[80];
  union semun
  {
    int               val;
    struct semid_ds   *buf;
    unsigned short    *array;
    struct seminfo    *__buf;
  } sem_un;
  struct shmid_ds shm_desc;
  struct semid_ds sem_desc;

  if (cont == NULL) return;
  if (!stream) stream = stdout;

  memset(&sem_un, 0, sizeof(union semun));
  memset(&shm_desc, 0, sizeof(struct shmid_ds));
  memset(&sem_desc, 0, sizeof(struct semid_ds));
  sem_un.buf = &sem_desc;

  (void)shmctl(cont->shm_id, IPC_STAT, &shm_desc);
  (void)semctl(cont->sem_id, 0, IPC_STAT, sem_un);
  fprintf(stream,"\n"
          "---------------------------------------------------\n"
          "             Info for segment:\n"
          "shm segment id:        %d\n"
          "semaphore id:          %d\n"
          "size of segment:       %u\n"
          "pointer of segment:    %p\n"
          "error message:         %s\n",
          cont->shm_id,
          cont->sem_id,
          cont->shm_size,
          cont->ptr,
          cont->err_msg);

  fprintf(stream,
          "\n             Stat of sem:\n"
          "Ownership and permissions:\n"
          "\tKey supplied to semget(2)=   %d\n"
          "\tEffective UID of owner=      %d\n"
          "\tEffective GID of owner=      %d\n"
          "\tEffective UID of creator=    %d\n"
          "\tEffective GID of creator=    %d\n"
          "\tPermissions=                 %#o\n",
          sem_desc.sem_perm.__key,
          (int)sem_desc.sem_perm.uid,
          (int)sem_desc.sem_perm.gid,
          (int)sem_desc.sem_perm.cuid,
          (int)sem_desc.sem_perm.cgid,
          sem_desc.sem_perm.mode);
  fprintf(stream,
          "\tSequence number=             %#o\n",
          sem_desc.sem_perm.__seq);
  strftime(buf,sizeof(buf),"%a %Y-%m-%d %H:%M:%S",localtime(&sem_desc.sem_otime));
  fprintf(stream,"Last semop time:               %s\n",buf);
  strftime(buf,sizeof(buf),"%a %Y-%m-%d %H:%M:%S",localtime(&sem_desc.sem_ctime));
  fprintf(stream,"Last change time:              %s\n",buf);
  fprintf(stream,"No. of semaphores in set:      %lu\n",sem_desc.sem_nsems);
  fprintf(stream,"Current value of zero sem:     %d\n",semctl(cont->sem_id,0,GETVAL));

  fprintf(stream,
          "\n             Stat of shm:\n"
          "Ownership and permissions:\n"
          "\tKey supplied to shmget(2)=   %d\n"
          "\tEffective UID of owner=      %d\n"
          "\tEffective GID of owner=      %d\n"
          "\tEffective UID of creator=    %d\n"
          "\tEffective GID of creator=    %d\n"
          "\tPermissions + SHM_DEST and\n"
          "\tSHM_LOCKED flags=            %#o\n",
          shm_desc.shm_perm.__key,
          (int)shm_desc.shm_perm.uid,
          (int)shm_desc.shm_perm.gid,
          (int)shm_desc.shm_perm.cuid,
          (int)shm_desc.shm_perm.cgid,
          shm_desc.shm_perm.mode);
  fprintf(stream,"Size of segment (bytes):       %d\n",(int)shm_desc.shm_segsz);
  strftime(buf,sizeof(buf),"%a %Y-%m-%d %H:%M:%S",localtime(&shm_desc.shm_atime));
  fprintf(stream,"Last attach time:              %s\n",buf);
  strftime(buf,sizeof(buf),"%a %Y-%m-%d %H:%M:%S",localtime(&shm_desc.shm_dtime));
  fprintf(stream,"Last detach time:              %s\n",buf);
  strftime(buf,sizeof(buf),"%a %Y-%m-%d %H:%M:%S",localtime(&shm_desc.shm_ctime));
  fprintf(stream,"Last change time:              %s\n",buf);
  fprintf(stream,
          "PID of creator:                %d\n"
          "PID of last shmat(2)/shmdt(2): %d\n"
          "No. of current attaches:       %d\n",
          shm_desc.shm_cpid,
          shm_desc.shm_lpid,
          (int)shm_desc.shm_nattch);
  return;
}
