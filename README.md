SHMCI API

SHMCI library provides common interface to shared memory

---
INSTALL

su
cd build
cmake ..
make install

---
TO TEST

su
cd build
cmake ..
make
../*.sh

---
NOTE

If your program fails due to this library error, try run as root.

---
EXAMPLE

Use -lshmci to compile your code,
see test sources and ./doc/ for more examples.

This example shows how to get access to shared memory,
write and read data and close connection.

#include <string.h>
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

/*  Some data to write  */
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

/*  Write data to shared memory and check for errors  */
    if( (err=shm_write(&link,text,0,size))<0 )
    {
        fprintf(stderr,"[%s:%d] Error #%d for link:\n%s",
                _FILE_,__LINE__,err,shm_strerror(&link));
        return -2;
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

---
ABOUT SHM_ADDR.H

You can store parameters for shm creation in that file to simplify usage.
The numeric constant should be unique for your project and 1 byte long.
Example of application:
  shm_connect(&shmlink,SHM_PATH,SHM_ADDR_NTPTIME,shm_size)

---
