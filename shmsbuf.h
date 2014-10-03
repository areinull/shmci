#ifndef _SHMSBUF_H_
#define _SHMSBUF_H_

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/*  maximum size of error message  */
#define SHMSBUF_EMSG_SIZE 0x100

static const uint16_t SHMSBUF_MAGIC = 0xA017u;
static const uint8_t SHMSBUF_VERSION = 0x03u;
static const uint16_t SHMSBUF_DATA_OFFSET = 40u;

/* return codes */
enum shmsbuf_ret
{
    NEW_CREATED = 3,
    CONN_TO_ANOTHER = 2,
    CONN_TO_EXISTING = 1,
    NOERR = 0,
    NULL_PTR = -1,
    BAD_ARGS = -2,
    SHM_ERR = -3,
    SEM_ERR = -4,
    NOT_SHM_SBUF = -5,
    SOME_ERR = -6,
    SEM_TIMEOUT = -7
};

/* offsets in sbuf header */
enum shmsbuf_offsets
{
    OFF_MAGIC =    0,
    OFF_OFF =      2,
    OFF_VER =      4,
    OFF_NMEMB =    8,
    OFF_ELSIZE =   16,
    OFF_CNT =      24,
    OFF_IDX =      32
};

struct timespec;

/* shm slide buffer context */
struct ShmSBuf
{
    int             shm_id;        /* shm segment id */
    int             sem_id;        /* semaphore id */
    size_t          shm_size;      /* size of shm segment */
    void*           ptr;           /* where segment is attached */
    char            err_msg[SHMSBUF_EMSG_SIZE]; /* last error */
    unsigned int    nmemb;         /* max number of elements in buffer (window size) */
    unsigned int    last_read;     /* last read element number (num. from 1, internal use) */
    size_t          elsize;        /* size of one element */
    size_t          dataoffset;    /* data offset in segment (size of header) */

    int (*deinit)(struct ShmSBuf *psbuf);
    int (*at)(struct ShmSBuf *psbuf, int idx, void *dst);
    int (*last)(struct ShmSBuf *psbuf, void *dst);
    int (*linearize)(struct ShmSBuf *psbuf, void *dst);
    int (*add)(struct ShmSBuf *psbuf, const void *src, unsigned int nmemb);
    int (*nread)(struct ShmSBuf *psbuf, void *dst, struct timespec *to);
};

/* ---- shm slide buffer interface functions ---- */
/* Initialize slide buffer (create new or get access to existing */
int shm_sbuf_init(struct ShmSBuf *psbuf,    /* context */
                  int proj_id,              /* unique number (8 low bits are significant) */
                  unsigned int nmemb,       /* buffer window size (in elements) */
                  size_t elsize);           /* element size */
/* Deinitialize slide buffer */
int shm_sbuf_deinit(struct ShmSBuf *psbuf);

/* Read one element from index position */
int shm_sbuf_at(struct ShmSBuf *psbuf,  /* context */
                int idx,                /* index of required element [-nmemb...0...nmemb-1] */
                void *dst);             /* destination pointer */
/* Read last element */
int shm_sbuf_last(struct ShmSBuf *psbuf, void *dst);
/* Read entire data from buffer in normal sequence */
int shm_sbuf_linearize(struct ShmSBuf *psbuf, void *dst);
/* Add nmemb elements to buffer */
int shm_sbuf_add(struct ShmSBuf *psbuf, const void *src, unsigned int nmemb);
/* Wait for new data in buffer and read it when available.
 * Return number of elements read from buffer (>0), negative value indicates error.
 * Make sure, that dst buffer is large enough (nmemb*elsize recommended)
 * 'to' specifies operation timeout, pass NULL for infinite wait */
int shm_sbuf_nread(struct ShmSBuf *psbuf, void *dst, struct timespec *to);

/* ---- shm slide buffer debugging functions ---- */
enum shmsbuf_info_type
{
    SIT_ALL,    /* print all info */
    SIT_SHMCI,  /* print info about shm segment and semaphore */
    SIT_BUF     /* print info about buffer (from buffer header) */
};

/* slide buffer header structure */
struct ShmSBuf_header
{
    uint16_t magic;         /* magic number */
    uint16_t dataoffset;    /* data offset (size of header) */
    uint8_t ver;            /* library version used upon creation */
    unsigned int nmemb;     /* max number of elements in buffer (window size) */
    size_t elsize;          /* size of one element */
    unsigned int cnt;       /* number of elements written to buffer since its creation */
    unsigned int idx;       /* index of element that follows the last written one */
};

/* Read everything from shm segment */
int shm_sbuf_raw(struct ShmSBuf *psbuf, void *dst);
/* Read entire datafield as is */
int shm_sbuf_data(struct ShmSBuf *psbuf, void *dst);
/* Fill header struc from buffer header */
int shm_sbuf_header(struct ShmSBuf *psbuf, struct ShmSBuf_header *head);
/* Print some info about shm segment, buffer or both */
int shm_sbuf_info(struct ShmSBuf *psbuf, enum shmsbuf_info_type type, FILE *fd);
/* Get the description of error code */
const char* shm_sbuf_error(int code);

#ifdef __cplusplus
}
#endif
#endif /* _SHMSBUF_H_ */
