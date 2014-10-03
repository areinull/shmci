%module ShmBind

%{
#include "shmci.h"
#include "shmsbuf.h"
#include "shm_addr.h"
typedef uint32_t     u32;
typedef struct
{
    u32 sec;  /*  seconds  */
    u32 psec; /*  "picoseconds"  */
} ntptime_t;
typedef struct
{
    ntptime_t ts;
    u32 Az;
    u32 El;
} adrive_pac_t;
%}

%include "shmci.h"
%include "shmsbuf.h"
%include "shm_addr.h"

%typemap(in) u32 {$1 = (u32) PyLong_AsUnsignedLong($input);}
typedef struct
{
    u32 sec;  /*  seconds  */
    u32 psec; /*  "picoseconds"  */
} ntptime_t;
typedef struct
{
    ntptime_t ts;
    u32 Az;
    u32 El;
} adrive_pac_t;

%inline %{
/* Create array of [size] elements */
adrive_pac_t* cu_array(int size)
{
    return (adrive_pac_t*)malloc(size*sizeof(adrive_pac_t));
}
/* Free array */
void free_cu_array(adrive_pac_t *p)
{
    free(p);
}
/* Assign element [elem] to [array] on [pos] position */
void cu_array_set(adrive_pac_t *array, char *elem, int pos)
{
    memcpy(array+pos, elem, sizeof(adrive_pac_t));
}
void mysleep(int secs)
{
    sleep(secs);
}
unsigned int get_u32(u32 *p)
{
        return *p;
}
unsigned int get_az(adrive_pac_t *pac)
{
    return pac->Az;
}
unsigned int get_za(adrive_pac_t *pac)
{
    return pac->El;
}
%}
