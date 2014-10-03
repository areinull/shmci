#include <shmci/shmci.h>
#include <vu_tools/vu_tools.h>

int main(void)
{
    int err; /* Error code */

    /*  'path' and 'ch' are string and integer that should
     *  be same for all programs using one shared memory segment
     */
    const char *path="/usr/include/stdio.h";
    const int ch='A';

    /*  Some data expected (need for size calculation)  */
    char *text="\nTest text to transfer\n\n";

    /*  Buffer for incoming data  */
    char output[255];

    /*  Use this link to work with shmci functions  */
    ShmLink link;

    int size;
    size=strlen(text)+1;

    /*  Get access to shared memory and check for errors  */
    if( (err=shm_connect(&link,path,ch,size))<0 )
    {
     fprintf(stderr,"[%s:%d] Error #%d for link:\n%s",
             _FILE_,__LINE__,err,shm_strerror(&link));
     return -1;
    }

    /*  Read data from shared memory and check for errors  */
    if( (err=shm_read(&link,output,0,size))<0 )
    {
     fprintf(stderr,"[%s:%d] Error #%d for link:\n%s",
             _FILE_,__LINE__,err,shm_strerror(&link));
     return -3;
    }

    /*  Close connection to shared memory and check for errors  */
    if( (err=shm_disconnect(&link))<0 )
    {
     fprintf(stderr,"[%s:%d] Error #%d for link:\n%s",
             _FILE_,__LINE__,err,shm_strerror(&link));
     return -4;
    }

    return 0;
}
