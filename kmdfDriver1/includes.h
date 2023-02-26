#pragma once

#define IOCTL_BOOSTER_SET_PRIORITY CTL_CODE(0x8000,0x800,METHOD_NEITHER,FILE_ANY_ACCESS)

typedef struct THREAD_DATA
{
	unsigned long thread_id;
	int priority;
}THREAD_DATA, *PTHREAD_DATA;