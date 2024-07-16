#ifndef __PEDESTAL_FUNCTION_H
#define __PEDESTAL_FUNCTION_H 

struct pedestal_operation {
    int  (*cpu_shut_prepare)(unsigned int* irqs, int num); 
    void (*cpu_shutdown)(void);
}; 

 
#endif

