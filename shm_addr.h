#ifndef __SHM_ADDR_H__
#define __SHM_ADDR_H__

#define SHM_PATH    "/usr/local/include/shmci/shmci.h"

/* Available numbers: 0x01 - 0xFF */
#define SHM_ADDR_NTPTIME         0x01
#define SHM_ADDR_NTPTIME_SYNC    0x02

#define SHM_ADDR_ADRIVE_INTER    0x20
#define SHM_ADDR_ADRIVE_CURRENT  0x21

#define SHM_ADDR_CPPB_PRI   0x30
#define SHM_ADDR_CPPB_FAST  0x35
#define SHM_ADDR_CPPB_QUAD  0x40

#define SHM_ADDR_INTER_ADRIVE       0x50
#define SHM_ADDR_INTER_BTO          0x60
    /* for shmsbuf from intercn to BTO */
#define SHM_ADDR_INTER_BTO_SHMSBUF  0x61
#define SHM_ADDR_BTO_INTER          0x70

#endif
