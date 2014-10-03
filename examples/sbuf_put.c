#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <vu_tools/vu_tools.h>
#include "shmsbuf.h"

#define NMEMB 16u

typedef struct ShmSBuf ShmSBuf_t;

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
    ShmSBuf_t buf_in;
    /* type of element for slide buffer */
    struct my_struct_s
    {
        uint64_t a;
        uint32_t b,
                 c;
    } my_struct;
    char i = 0;

    (void)signal(SIGINT, signal_handler);
    (void)signal(SIGTERM, signal_handler);

    /* Buffer initialization:
     * buf_in - simulates transmitting process
     */
    if ((ret = shm_sbuf_init(&buf_in, 0x05, NMEMB, sizeof(struct my_struct_s)))
        >= 0)
    {
        printf("buf_in init succeded with %d\t%s\n", ret, shm_sbuf_error(ret));
    }
    else
    {
        printf("shm_sbuf_init failed with %d\t%s\n", ret, shm_sbuf_error(ret));
        printf("shmsbuf_err: %s\n", buf_in.err_msg);
        return -1;
    }

    for (; !g_stop; ++i)
    {
        /* Generate new data */
        my_struct.a = i*100;
        my_struct.b = i*10;
        my_struct.c = i+1;

        /* Write data to shm buffer */
        if ((ret = buf_in.add(&buf_in, &my_struct, 1)) < 0)
        {
            printf("buf.add failed with %d\t%s\n", ret,
                   shm_sbuf_error(ret));
            printf("shmsbuf_err: %s\n", buf_in.err_msg);
            break;
        }
        uSleep(500000);
    }

    /* Deinitialize shm buffer */
    if ((ret = buf_in.deinit(&buf_in)) >= 0)
    {
        printf("buf_in.deinit succeded with %d\t%s\n", ret,
               shm_sbuf_error(ret));
    }
    else
    {
        printf("buf_in.deinit failed with %d\t%s\n", ret, shm_sbuf_error(ret));
        printf("shmsbuf_err: %s\n", buf_in.err_msg);
    }
    return 0;
}
