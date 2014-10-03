#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "shmci.h"

int main(void)
{
    const char *path="/usr/include/stdio.h";
    const int ch='A';
    char *text="\nTest text to transfer\n\n";
    char output[255];
    ShmLink link;
    int size,err;

    size=strlen(text)+1;

    /* Allocate shared memory segment with key, created with 'path' and 'ch'
       parameters using ftok(), and size of 'size' byte.
    */
    if( (err=shm_connect(&link,path,ch,size))<0 )
    {
        fprintf(stderr,"[%s:%d] Error #%d for shm_link:\n%s",
                __FILE__,__LINE__,err,shm_strerror(&link));
        return -1;
    }
    /* Write data of 'size' byte long pointed by 'text' to shared memory
       segment associated with 'link' parameter, starting with zero offset
       (i.e. the beginning of allocated shared memory segment).
    */
    if( (err=shm_write(&link,text,0,size))<0 )
    {
        fprintf(stderr,"[%s:%d] Error #%d for link:\n%s",
                __FILE__,__LINE__,err,shm_strerror(&link));
        return -2;
    }
    sleep(1);
    /*  Read data of 'size' byte long from shared memory segment associated
        with 'link' parameter, starting with zero offset (i.e. the beginning
        of allocated shared memory segment), to the location pointed by
        'output'.
    */
    if( (err=shm_read(&link,output,0,size))<0 )
    {
        fprintf(stderr,"[%s:%d] Error #%d for link:\n%s",
                __FILE__,__LINE__,err,shm_strerror(&link));
        return -3;
    }
    /* Detach process from shared memory segment associated with 'link' and
       destroy segment, because it was the last process attached to it.
    */
    if( (err=shm_disconnect(&link))<0 )
    {
        fprintf(stderr,"[%s:%d] Error #%d for link:\n%s",
                __FILE__,__LINE__,err,shm_strerror(&link));
        return -4;
    }
    /* Check for identity of source and received data.
    */
    if( strncmp(text,output,size-1) )
    {
        fprintf(stderr,"[%s:%d] Error for link:\n"
                       "incorrect transfer\n",
                       __FILE__,__LINE__);
        return -5;
    }

    return 0;
}
