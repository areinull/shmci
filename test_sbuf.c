#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <vu_tools/vu_tools.h>
#include "shm_addr.h"
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
    char *data;
//     char array[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    char i = 0;

    printf("Beginning\n");
    (void)signal(SIGINT, signal_handler);
    (void)signal(SIGTERM, signal_handler);

    /* Buffer initialization:
     * buf_in - simulates transmitting process
     * buf_out - simulates receiving process
     */
    if ((ret = shm_sbuf_init(&buf_in, SHM_ADDR_NTPTIME, 16, sizeof(char))) >= 0)
    {
        printf("buf_in init succeded with %d\t%s\n", ret, shm_sbuf_error(ret));
    }
    else
    {
        printf("shm_sbuf_init failed with %d\t%s\n", ret, shm_sbuf_error(ret));
        printf("shmsbuf_err: %s\n", buf_in.err_msg);
        return -1;
    }
    if ((ret = shm_sbuf_init(&buf_out, SHM_ADDR_NTPTIME, 16, sizeof(char)))
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

/* Memory allocation for receiving buffer (simulates program internal buffer) */
    data = (char*)malloc(buf_out.elsize*buf_out.nmemb);
    if (!data)
        printf("Memory allocation for data failed\n");
    else
    {
        /* Check read with empty buffer */
        puts("\nCheck read with empty buffer");
        if ((ret = buf_out.last(&buf_out, data)) < 0)
        {
            printf("buf.last returned %d: %s (%d expected)\n",
                   ret, shm_sbuf_error(ret), SOME_ERR);
        }
        else
        {
            printf("Error: last: %02x (nothing expected)\n", data[0]);
        }
        printf("last_read = %u\n", buf_out.last_read);
        data[0] = 0;
        for (; !g_stop; ++i)
        {
            /* Write data to shm buffer */
            if ((ret = buf_in.add(&buf_in, &i, 1)) < 0)
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
                hexdump(data, buf_out.elsize*buf_out.nmemb, stdout);
            }
            printf("last_read = %u\n", buf_out.last_read);
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
                hexdump(data, buf_out.elsize*buf_out.nmemb, stdout);
            }
            printf("last_read = %u\n", buf_out.last_read);
            memset(data, 0, buf_out.elsize*buf_out.nmemb);

            /* Read 3-rd (for example) element of buffer
             * Note: this call fails if there are less than 4 elements already
             * written to buffer */
            if ((ret = buf_out.at(&buf_out, 3, data)) < 0)
            {
                printf("buf.at failed with %d\t%s\n", ret,
                       shm_sbuf_error(ret));
                printf("shmsbuf_err: %s\n", buf_out.err_msg);
            }
            else
            {
                printf("At 3: %02x\n", data[0]);
            }
            printf("last_read = %u\n", buf_out.last_read);
            data[0]=0;
            
            /* Read last element of buffer */
            if ((ret = buf_out.last(&buf_out, data)) < 0)
            {
                printf("buf.last failed with %d\t%s\n", ret,
                       shm_sbuf_error(ret));
                printf("shmsbuf_err: %s\n", buf_out.err_msg);
            }
            else
            {
                printf("Last: %02x\n", data[0]);
            }
            printf("last_read = %u\n", buf_out.last_read);
            data[0]=0;

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
                printf("At -4: %02x\n", data[0]);
            }
            printf("last_read = %u\n", buf_out.last_read);
            data[0]=0;
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