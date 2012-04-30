#ifndef _ADJ_TIME_H
#define _ADJ_TIME_H

#include <sys/time.h>

int adjust_master_prepare(void);
int adjust_master_addslave(const unsigned char ip[4], const struct timeval *time);
int adjust_master_calcandsend(void);

int adjust_slave_clock(const struct timeval *diff);

#endif
