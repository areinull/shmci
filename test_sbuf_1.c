#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <signal.h>
#include <vu_tools/vu_tools.h>
#include "shmsbuf.h"

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
    ShmSBuf_t buf_in, buf_out;
    struct my_struct_s
    {
        uint64_t a;
        uint32_t b,
                 c;
    } my_struct, *data;
    char i = 0;

    printf("Beginning\n");
    (void)signal(SIGINT, signal_handler);
    (void)signal(SIGTERM, signal_handler);

    /* Buffer initialization:
     * buf_in - simulates transmitting process
     * buf_out - simulates receiving process
     */
    if ((ret = shm_sbuf_init(&buf_in, 0x05, 16, sizeof(struct my_struct_s)))
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
    if ((ret = shm_sbuf_init(&buf_out,  0x05, 16, sizeof(struct my_struct_s)))
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

    data = (struct my_struct_s*)malloc(buf_out.elsize*buf_out.nmemb);
    if (!data)
        printf("Memory allocation for data failed\n");
    else
    {
        for (; !g_stop; ++i)
        {
            /* Write data to shm buffer */
            my_struct.a = i*100;
            my_struct.b = i*10;
            my_struct.c = i+1;
            if ((ret = buf_in.add(&buf_in, &my_struct, 1)) < 0)
            {
                printf("buf.add failed with %d\t%s\n", ret,
                       shm_sbuf_error(ret));
                printf("shmsbuf_err: %s\n", buf_in.err_msg);
                break;
            }
//             if ((ret = buf_in.add(&buf_in, &array, 16)) < 0)
//             {
//                 printf("buf.add failed with %d\t%s\n", ret,
//                        shm_sbuf_error(ret));
//                 printf("shmsbuf_err: %s\n", buf_in.err_msg);
//                 break;
//             }
            /* Read data in 'raw' format */
            if ((ret = shm_sbuf_data(&buf_out, data)) < 0)
            {
                printf("shm_sbuf_data failed with %d\t%s\n", ret,
                       shm_sbuf_error(ret));
                printf("shmsbuf_err: %s\n", buf_out.err_msg);
            }
            else
            {
                printf("\nData:\n");
                hexdump((void*)data, buf_out.elsize*buf_out.nmemb, stdout);
            }
            memset(data, 0, buf_out.elsize*buf_out.nmemb);

            /* Read data in right sequence */
            if ((ret = buf_out.linearize(&buf_out, data)) < 0)
            {
                printf("buf.linearize failed with %d\t%s\n", ret,
                       shm_sbuf_error(ret));
                printf("shmsbuf_err: %s\n", buf_out.err_msg);
            }
            else
            {
                printf("Linear:\n");
                hexdump((void*)data, buf_out.elsize*buf_out.nmemb, stdout);
            }
            memset(data, 0, buf_out.elsize*buf_out.nmemb);

            /* Read last element of buffer */
            if ((ret = buf_out.last(&buf_out, data)) < 0)
            {
                printf("buf.last failed with %d\t%s\n", ret, shm_sbuf_error(ret));
                printf("shmsbuf_err: %s\n", buf_out.err_msg);
            }
            else
            {
                printf("Last: a = %"PRIu64"  b = %u  c = %u\n",
                       data[0].a, data[0].b, data[0].c);
            }

            /* Read 3-rd (for example) element of buffer
             * Note: this call fails if there are less than 4 elements already
             * written to buffer */
            if ((ret = buf_out.at(&buf_out, 3, data)) < 0)
            {
                printf("buf.at failed with %d\t%s\n", ret, shm_sbuf_error(ret));
                printf("shmsbuf_err: %s\n", buf_out.err_msg);
            }
            else
            {
                printf("At 3: a = %"PRIu64"  b = %u  c = %u\n",
                       data[0].a, data[0].b, data[0].c);
            }

            /* Read -4-rd (for example) element of buffer
             * Note: this call fails if there are less than 4 elements already
             * written to buffer */
            if ((ret = buf_out.at(&buf_out, -4, data)) < 0)
            {
                printf("buf.at failed with %d\t%s\n", ret, shm_sbuf_error(ret));
                printf("shmsbuf_err: %s\n", buf_out.err_msg);
            }
            else
            {
                printf("At -4: a = %"PRIu64"  b = %u  c = %u\n",
                       data[0].a, data[0].b, data[0].c);
            }
            uSleep(500000);
        }
        free(data);
    }

    /* Print info about buffer and shm segment */
    if ((ret = shm_sbuf_info(&buf_in, SIT_ALL, stdout)) < 0)
    {
        printf("shm_sbuf_info failed with %d\t%s\n", ret, shm_sbuf_error(ret));
        printf("shmsbuf_err: %s\n", buf_in.err_msg);
    }

    /* Deinitialize shm buffers */
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
