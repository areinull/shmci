#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "shmci.h"

int main(void)
{
    const char *path="/usr/include/stdio.h";
    const int ch='A';
    char *text="\nThis is the test text and integer: ";
    ShmLink link;
    int size1,size2,err,i=0;

    size1=strlen(text)+1;
    size2=sizeof(int);

    /* Allocate shared memory segment with key, created with 'path' and 'ch'
       parameters using ftok(),(it's same as in test2_0) and size of 'size' byte.
       It's assumed that this process creates segment and test2_0 attaches to
       it later.
    */
    if( (err=shm_connect(&link,path,ch,size1+size2))<0 )
    {
        fprintf(stderr,"[%s:%d] Error #%d for shm_link:\n%s",
                    __FILE__,__LINE__,err,shm_strerror(&link));
        return -1;
    }
    for(i=0;i<30;i++)
    {
    /* Data structure transfering to test2_0 is as follows:
       0                             36                               40
       | text of 37 byte long (size1) | integer of 4 byte long (size2) |
       
       Write text string of 'size1=37' byte long pointed by 'text' to shared
       memory segment associated with 'link' parameter, starting with zero offset
       (i.e. the beginning of allocated shared memory segment).
    */
        if( (err=shm_write(&link,text,0,size1))<0 )
        {
            fprintf(stderr,"[%s:%d] Error #%d for link:\n%s",
                __FILE__,__LINE__,err,shm_strerror(&link));
            return -2;
        }
    /*  Write integer of 'size2=4' byte long pointed by '&i' to shared memory
       segment associated with 'link' parameter, starting with 'size1' offset
       (i.e. right after the text string).
    */
        if( (err=shm_write(&link,&i,size1,size2))<0 )
        {
            fprintf(stderr,"[%s:%d] Error #%d for link:\n%s",
                __FILE__,__LINE__,err,shm_strerror(&link));
            return -3;
        }
        sleep(1);
    }
    /* Detach process from shared memory segment associated with 'link'.
       Destroy segment, if it was the last process attached to it
       (if test2_0 exited earlier than this process).
    */
    if( (err=shm_disconnect(&link))<0 )
    {
        fprintf(stderr,"[%s:%d] Error #%d for link:\n%s",
                __FILE__,__LINE__,err,shm_strerror(&link));
        return -4;
    }

    return 0;
}
