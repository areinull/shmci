#ifndef __SHMCI_H__
#define __SHMCI_H__

#include <stdio.h>

#ifdef __cplusplus
 extern "C" {
#endif

/*  maximum size of error message  */
#define SHMCI_EMSG_SIZE 0x100

/* shm context */
typedef struct
{
  int             shm_id;        /* shm segment id            */
  int             sem_id;        /* semaphore id              */
  unsigned int    shm_size;      /* size of shm segment       */
  void            *ptr;          /* where segment is attached */
  char            err_msg[SHMCI_EMSG_SIZE];
} ShmLink;

/* ----------------------- shm create and delete --------------------------- */

/* Break connection to shm segment 'cont'.
   Delete shm segment if it was the last client using this shm segment.
   Return 0 on success.
*/
int shm_disconnect(ShmLink* cont);

/* Establish connection to existing shm segment 'cont'
   using its unique 'key_path' and 'key_int'.
   Otherwise create new shm segment 'cont' of 'size' bytes
   long. 'size' is ignored in first case.
*/
int shm_connect(ShmLink *cont,
                const char *key_path,
                const int key_int,
                unsigned int size);

int shm_clean(ShmLink *cont);

/* ---------------------- shm low-level functions -------------------------- */

/* Write 'size' bytes of data, starting
   from 'offset', to shm segment 'cont' from
   location given by 'ptr'.
*/
int shm_write(ShmLink *cont,
              const void *ptr,
              unsigned int offset,
              unsigned int size);

/* Read 'size' bytes of data, starting
   from 'offset', from shm segment 'cont' to
   location given by 'ptr'.
*/
int shm_read(ShmLink *cont,
             void *ptr,
             unsigned int offset,
             unsigned int size);

/* ------------------------- shm misc functions ---------------------------- */

/* Return pointer to static error message related with 'cont'.
   Size of returned string <= SHMCI_EMSG_SIZE
   Modification of returned string is generally bad idea.
*/
const char* shm_strerror(ShmLink *cont);

/* Print some info about segment 'cont' to 'stream' */
void shm_printinfo(ShmLink *cont, FILE *stream);

#ifdef __cplusplus
 }
#endif
#endif  /* __SHMCI_H__ */
