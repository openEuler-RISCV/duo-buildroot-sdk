/*
 * Copyright (c) 2024-2024 Huawei Technologies Co., Ltd. All rights reserved.
 *
 * UniProton is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * Create: 2024-02-22
 * Description: main函数入口,初始化内核以及执行console_init()函数
 */
#include "prt_task.h"
#include "prt_config.h"
#include "prt_typedef.h"
#include "csr.h"
#include "arch_time.h"
#include "irq.h"
#include "printf.h"
#include "prt_exc.h"
// 初始配置函数入口
extern S32 OsConfigStart(void);

// milkvduo little pre system init 
extern void pre_system_init(void);

// milkvduo litte post system init
extern void post_system_init(void);

// 用户可以使用的接口，需要写入mtvec
extern void trap(); 


U8 ucHeap[ HEAP_SIZE ] __attribute__ ( ( section( ".heap" ) ) );
extern char _bss[];
extern char _ebss[];
extern char _heap_start[];
extern char _heap_end[];
void OsHwInit()
{
    //clear mstatus mie
    clear_csr(mstatus, MSTATUS_MIE);
    //open mie -> meie , mtie, msie
    set_csr(mie, MIP_MEIE);
    set_csr(mie, MIP_MTIE);
    set_csr(mie, MIP_MSIE);
    U64 x = (U64)trap;
    write_csr(mtvec, x);
    U64 now_time = GetSysTime();
    U64 next_time = now_time + OS_TICK_PER_SECOND;
    *((U32*)CLINT_TIMECMPL0) = (U32)(next_time & 0xffffffff);
    *((U32*)CLINT_TIMECMPH0) = (U32)((next_time >> 32) & 0xffffffff);
    printf("[UniProton ]: OsHwInit done! x: 0x%p mtvec: 0x%p\n", x, read_csr(mtvec));
    return;
}


U32 PRT_HardDrvInit(void)
{
    
    // do your driver init 
   
    printf("[UniProton ]: HardDrvInit done !\n");

    return OS_OK;
}


extern void main_cvirtos(void);
U32 PRT_AppInit(void)
{
    printf("[UniProton]: app init start !\n");
    main_cvirtos();
    printf("[UniProton]: app init done !\n");
    return OS_OK;
}

U32 Exc_Hook(struct ExcInfo *excInfo) 
{
	return -1;
}

int main()
{
    //arch_usleep(10000000);
    pre_system_init();
    post_system_init();
    printf("[UniProton]: start to init OsConfig!\n");
    return OsConfigStart();
}

