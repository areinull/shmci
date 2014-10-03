/* test3_0 and test3_1 programs work just like test2_0 and test2_1
   with few exceptions:
   - unlike test2*, test3* work with three shared memory segments;
   - data transfers in both directions.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "shmci.h"

int main(void)
{
    const char *path1="/usr/include/stdio.h";
    const int ch1='A';
    const char *path2="/usr/include/stdio.h";
    const int ch2='B';
    const char *path3="/usr/include/stdio.h";
    const int ch3='C';
    char *text3="\nThis is the test text #3 and integer: ";
    char text1[256],text2[256];
    ShmLink link1,link2,link3;
    int size1,size2,err,i=0,res1=0,res2=0;

    size1=strlen(text3)+1;
    size2=sizeof(int);

    if( (err=shm_connect(&link3,path3,ch3,size1+size2))<0 )
    {
        fprintf(stderr,"[%s:%d] Error #%d for shm_link:\n%s",
                __FILE__,__LINE__,err,shm_strerror(&link3));
        return -1;
    }
    sleep(1);
    if( (err=shm_connect(&link2,path2,ch2,size1+size2))<0 )
    {
        fprintf(stderr,"[%s:%d] Error #%d for shm_link:\n%s",
                __FILE__,__LINE__,err,shm_strerror(&link2));
        return -1;
    }
    if( (err=shm_connect(&link1,path1,ch1,size1+size2))<0 )
    {
        fprintf(stderr,"[%s:%d] Error #%d for shm_link:\n%s",
                __FILE__,__LINE__,err,shm_strerror(&link1));
        return -1;
    }
    for(i=0;i<10;i++)
    {
        if( (err=shm_write(&link3,text3,0,size1))<0 )
        {
            fprintf(stderr,"[%s:%d] Error #%d for link:\n%s",
                __FILE__,__LINE__,err,shm_strerror(&link3));
            return -2;
        }
        if( (err=shm_write(&link3,&i,size1,size2))<0 )
        {
            fprintf(stderr,"[%s:%d] Error #%d for link:\n%s",
                __FILE__,__LINE__,err,shm_strerror(&link3));
            return -2;
        }

        if( (err=shm_read(&link1,text1,0,size1))<0 )
        {
            fprintf(stderr,"[%s:%d] Error #%d for link:\n%s",
                __FILE__,__LINE__,err,shm_strerror(&link1));
            return -2;
        }
        if( (err=shm_read(&link1,&res1,size1,size2))<0 )
        {
            fprintf(stderr,"[%s:%d] Error #%d for link:\n%s",
                __FILE__,__LINE__,err,shm_strerror(&link1));
            return -2;
        }
        if( (err=shm_read(&link2,text2,0,size1))<0 )
        {
            fprintf(stderr,"[%s:%d] Error #%d for link:\n%s",
                __FILE__,__LINE__,err,shm_strerror(&link2));
            return -2;
        }
        if( (err=shm_read(&link2,&res2,size1,size2))<0 )
        {
            fprintf(stderr,"[%s:%d] Error #%d for link:\n%s",
                __FILE__,__LINE__,err,shm_strerror(&link2));
            return -2;
        }
        sleep(2);
    }
    if( (err=shm_disconnect(&link1))<0 )
    {
        fprintf(stderr,"[%s:%d] Error #%d for link:\n%s",
                __FILE__,__LINE__,err,shm_strerror(&link1));
        return -4;
    }
    if( (err=shm_disconnect(&link2))<0 )
    {
        fprintf(stderr,"[%s:%d] Error #%d for link:\n%s",
                __FILE__,__LINE__,err,shm_strerror(&link2));
        return -4;
    }
    if( (err=shm_disconnect(&link3))<0 )
    {
        fprintf(stderr,"[%s:%d] Error #%d for link:\n%s",
                __FILE__,__LINE__,err,shm_strerror(&link3));
        return -4;
    }

    return 0;
}
