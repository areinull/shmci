#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <vu_tools/vu_tools.h>
#include "shmsbuf.h"

#define NMEMB 16u

typedef struct ShmSBuf ShmSBuf_t;

static struct timespec timeout = {3, 0};
static volatile sig_atomic_t g_stop = 0;

/*  signal handler to exit on Ctrl-C  */
static void signal_handler(int nsig)
{
    (void)nsig;
    g_stop = 1;
}

int main(void)
{
    int ret;
    ShmSBuf_t buf_out;
    /* type of element for slide buffer */
    struct my_struct_s
    {
        uint64_t a;
        uint32_t b,
                 c;
    } *data;
    char i = 0;

    (void)signal(SIGINT, signal_handler);
    (void)signal(SIGTERM, signal_handler);

    /* Buffer initialization:
     * buf_out - simulates receiving process
     */
    if ((ret = shm_sbuf_init(&buf_out,  0x05, NMEMB, sizeof(struct my_struct_s)))
        >= 0)
    {
        printf("buf_out init succeded with %d\t%s\n", ret, shm_sbuf_error(ret));
    }
    else
    {
        printf("shm_sbuf_init failed with %d\t%s\n", ret, shm_sbuf_error(ret));
        printf("shmsbuf_err: %s\n", buf_out.err_msg);
        return -2;
    }

    /* Allocate space for data receiving */
    data = (struct my_struct_s*)malloc(buf_out.elsize*buf_out.nmemb);
    if (!data)
        printf("Memory allocation for data failed\n");
    else
    {
        for (; !g_stop; ++i)
        {
            /* Read new data from buffer */
            if ((ret = buf_out.nread(&buf_out, data, &timeout)) < 0)
            {
                printf("buf.nread failed with %d\t%s\n", ret, shm_sbuf_error(ret));
                printf("shmsbuf_err: %s\n", buf_out.err_msg);
            }
            else
            {
                printf("Received %d new elements\n", ret);
            }
        }
        free(data);
    }

    /* Deinitialize shm buffer */
    if ((ret = buf_out.deinit(&buf_out)) >= 0)
    {
        printf("buf_out.deinit succeded with %d\t%s\n", ret,
               shm_sbuf_error(ret));
    }
    else
    {
        printf("buf_out.deinit failed with %d\t%s\n", ret, shm_sbuf_error(ret));
        printf("shmsbuf_err: %s\n", buf_out.err_msg);
    }
    return 0;
}
