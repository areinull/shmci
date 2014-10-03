#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "shmci.h"

int main(void)
{
    const char *path="/usr/include/stdio.h";
    const int ch='A';
    char text[255];
    ShmLink link;
    int err,i,res=0;

    /* Attach shared memory segment with key, created with 'path' and 'ch'
       parameters using ftok(),(it's same as in test2_1) to process' address
       space. Assuming that segment has been already created by test2_1,
       third parameter value is insignificant.
    */
    if( (err=shm_connect(&link,path,ch,0))<0 )
    {
        fprintf(stderr,"[%s:%d] Error #%d for shm_link:\n%s",
                    __FILE__,__LINE__,err,shm_strerror(&link));
        return -1;
    }
    for(i=0;i<5;i++)
    {
    /* Data structure transfered from test2_1 is as follows:
       0                         36                        40
       |   text of 37 byte long   | integer of 4 byte long |

       Read data of 37 byte long (text string) from shared memory segment
       associated with 'link' parameter, starting with zero offset (i.e. the
       beginning the shared memory segment), to the location pointed by 'text'.
    */
        if( (err=shm_read(&link,text,0,37))<0 )
        {
            fprintf(stderr,"[%s:%d] Error #%d for link:\n%s",
                __FILE__,__LINE__,err,shm_strerror(&link));
            return -2;
        }
    /* Read data of sizeof(int)=4 byte long from shared memory segment
       associated with 'link' parameter, starting with 37 byte offset (i.e. the
       offset of integer in the segment), to the location pointed by '&res'.
    */
        if( (err=shm_read(&link,&res,37,sizeof(int)))<0 )
        {
            fprintf(stderr,"[%s:%d] Error #%d for link:\n%s",
                __FILE__,__LINE__,err,shm_strerror(&link));
            return -3;
        }
        sleep(2);
    }
    /* Detach process from shared memory segment associated with 'link'.
       Destroy segment, if it was the last process attached to it
       (if test2_1 exited earlier than this process).
    */
    if( (err=shm_disconnect(&link))<0 )
    {
        fprintf(stderr,"[%s:%d] Error #%d for link:\n%s",
                __FILE__,__LINE__,err,shm_strerror(&link));
        return -4;
    }

    return 0;
}
