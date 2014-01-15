/*
 * Copyright (c) 2011-2012, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 *  ======== test_omx.c ========
 *
 *  Example of setting up an "OMX" service with the ServiceMgr, allowing clients
 *  to instantiate OMX instances.
 *
 *  Works with the test_omx.c Linux user space application over the rpmsg_omx\
 *  driver.
 */

#include <xdc/std.h>
#include <xdc/cfg/global.h>
#include <xdc/runtime/System.h>
#include <xdc/runtime/Diags.h>

#include <ti/ipc/MultiProc.h>
#include <ti/sysbios/BIOS.h>

#include <ti/grcm/RcmTypes.h>
#include <ti/grcm/RcmServer.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <ti/srvmgr/ServiceMgr.h>
#include <ti/srvmgr/rpmsg_omx.h>
#include <ti/srvmgr/omx_packet.h>
#include <ti/sysbios/hal/Cache.h>
#include <ti/sysbios/knl/Event.h>
#include <xdc/runtime/Error.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/gates/hwspinlock/HwSpinlock.h>

extern String HwSpinlockStatesName[] ;


/* Turn on/off printf's */
#define CHATTER 1

#define SIZE 480

typedef struct {
    UInt32 a;
    UInt32 b;
    UInt32 c;
    UInt32 start_indx ;
    UInt32 end_indx ;
} FxnArgs;
	
typedef struct {
	int(*buffer1)[SIZE] ;
	int(*buffer2)[SIZE] ;
	int(*buffer3)[SIZE] ;
	int start_indx ;
	int end_indx ;
	int type ;
} taskArgs ;

Task_Params taskParams0, taskParams1 ;
Task_Handle task0, task1 ;
Error_Block eb0,eb1 ;
taskArgs t1, t2;


/* Legacy function to allow Linux side rpmsg sample tests to work: */
extern void start_ping_tasks();
extern void start_resmgr_task();
extern void start_hwSpinlock_task();

/*
 * OMX packet expected to have its data payload start with a payload of
 * this value. Need to export this properly in a meaningful header file on
 * both HLOS and RTOS sides
 */
typedef enum {
    RPC_OMX_MAP_INFO_NONE       = 0,
    RPC_OMX_MAP_INFO_ONE_BUF    = 1,
    RPC_OMX_MAP_INFO_TWO_BUF    = 2,
    RPC_OMX_MAP_INFO_THREE_BUF  = 3,
    RPC_OMX_MAP_INFO_MAX        = 0x7FFFFFFF
} map_info_type;

void callBarrier(UInt32 indx) ;

static Int32 fxnTest1(UInt32 size, UInt32 *data) ;

/* ==========================================================================
 * OMX Fxns, adapted from rpc_omx_skel.c.
 *
 * These defines are to illustrate reuse of RPC_SKEL fxns with ServiceMgr.
 *===========================================================================*/

#define H264_DECODER_NAME   "H264_decoder"

#define OMX_VIDEO_THREAD_PRIORITY    5

typedef Int32  RPC_OMX_ERRORTYPE;
typedef UInt32 OMX_HANDLETYPE;

static RPC_OMX_ERRORTYPE RPC_SKEL_GetHandle(Void *, UInt32 size, UInt32 *data);
static RPC_OMX_ERRORTYPE RPC_SKEL_SetParameter(UInt32 size, UInt32 *data);
static RPC_OMX_ERRORTYPE RPC_SKEL_GetParameter(UInt32 size, UInt32 *data);

Event_Handle edgeDetectEvent;

/* RcmServer static function table */
static RcmServer_FxnDesc OMXServerFxnAry[] = {
//    {"RPC_SKEL_GetHandle"   , RPC_SKEL_GetHandle},  // Set at runtime.
    {"RPC_SKEL_GetHandle"   , NULL},
    {"RPC_SKEL_SetParameter", RPC_SKEL_SetParameter},
    {"RPC_SKEL_GetParameter", RPC_SKEL_GetParameter},
    {"fxnTest1", fxnTest1},
};

#define OMXServerFxnAryLen (sizeof OMXServerFxnAry / sizeof OMXServerFxnAry[0])

static const RcmServer_FxnDescAry OMXServer_fxnTab = {
    OMXServerFxnAryLen,
    OMXServerFxnAry
};


static RPC_OMX_ERRORTYPE RPC_SKEL_SetParameter(UInt32 size, UInt32 *data)
{
#if CHATTER
    System_printf("RPC_SKEL_SetParameter: Called\n");
#endif

    return(0);
}

static RPC_OMX_ERRORTYPE RPC_SKEL_GetParameter(UInt32 size, UInt32 *data)
{
#if CHATTER
    System_printf("RPC_SKEL_GetParameter: Called\n");
#endif

    return(0);
}

#define CALLBACK_DATA      "OMX_Callback"
#define PAYLOAD_SIZE       sizeof(CALLBACK_DATA)
#define CALLBACK_DATA_SIZE (HDRSIZE + OMXPACKETSIZE + PAYLOAD_SIZE)

static RPC_OMX_ERRORTYPE RPC_SKEL_GetHandle(Void *srvc, UInt32 size,
                                           UInt32 *data)
{
    char              cComponentName[128] = {0};
    OMX_HANDLETYPE    hComp;
    Char              cb_data[HDRSIZE + OMXPACKETSIZE + PAYLOAD_SIZE] =  {0};

    /*
     * Note: Currently, rpmsg_omx linux driver expects an omx_msg_hdr in front
     * of the omx_packet data, so we allow space for this:
     */
    struct omx_msg_hdr * hdr = (struct omx_msg_hdr *)cb_data;
    struct omx_packet  * packet = (struct omx_packet *)hdr->data;


    //Marshalled:[>offset(cParameterName)|>pAppData|>offset(RcmServerName)|>pid|
    //>--cComponentName--|>--CallingCorercmServerName--|
    //<hComp]

    strcpy(cComponentName, (char *)data + sizeof(map_info_type));

#if CHATTER
    System_printf("RPC_SKEL_GetHandle: Component Name received: %s\n",
                  cComponentName);
#endif

    /* Simulate sending an async OMX callback message, passing an omx_packet
     * structure.
     */
    packet->msg_id  = 99;   // Set to indicate callback instance, buffer id, etc.
    packet->fxn_idx = 5;    // Set to indicate callback fxn
    packet->data_size = PAYLOAD_SIZE;
    strcpy((char *)packet->data, CALLBACK_DATA);

#if CHATTER
    System_printf("RPC_SKEL_GetHandle: Sending callback message id: %d, "
                  "fxn_id: %d, data: %s\n",
                  packet->msg_id, packet->fxn_idx, packet->data);
#endif
    ServiceMgr_send(srvc, cb_data, CALLBACK_DATA_SIZE);

    /* Call OMX_Get_Handle() and return handle for future calls. */
    //eCompReturn = OMX_GetHandle(&hComp, (OMX_STRING)&cComponentName[0], pAppData,&rpcCallBackInfo);
    hComp = 0x5C0FFEE5;
    data[0] = hComp;

#if CHATTER
    System_printf("RPC_SKEL_GetHandle: returning hComp: 0x%x\n", hComp);
#endif

    return(0);
}

void wb_invalidate_caches(void *ib1, void* ib2, void* ib3, int num_bytes)
{
	Cache_wbInv (ib1, num_bytes, Cache_Type_ALL, FALSE);
	Cache_wbInv (ib2, num_bytes, Cache_Type_ALL, FALSE);
	Cache_wbInv (ib3, num_bytes, Cache_Type_ALL, FALSE);
}

void invalidate_caches(void *ib1, void* ib2, void* ib3, int num_bytes)
{
	Cache_inv (ib1, SIZE*SIZE*4, Cache_Type_ALL, FALSE) ;
	Cache_inv (ib2, SIZE*SIZE*4, Cache_Type_ALL, FALSE) ;
	Cache_inv (ib3, SIZE*SIZE*4, Cache_Type_ALL, FALSE);
}

void multiply(int* restrict A, int* restrict B, int* restrict C, int start, int end)
{
        int i,j,k;
        for (i = start ; i < end ; i++)
        {   
                for (j = 0; j < SIZE; j++)
                {   
                        C[i*SIZE+j] = 0;
                        for ( k = 0; k < SIZE; k++)
                                C[i*SIZE+j] += A[i*SIZE+k]*B[k*SIZE+j];
                }   
        }   
}

int sum(int size, int beta)
{
        int i = 0 ; 
        int alpha = 0 ; 
        while(i++ < size) {
                alpha = alpha + beta ;
        }   
        return alpha ;
}

void common_wrapper(UArg arg0, UArg arg1)
{
	taskArgs* t = (taskArgs*)arg0 ;
	int tid = (int)arg1 ;

	t->type = sum(0x3FFFFFFF, 23) ;
	multiply((int*)(t->buffer1), (int*)(t->buffer2), (int*)(t->buffer3), t->start_indx, t->end_indx) ;

	if(tid == 0)
		Event_post(edgeDetectEvent, Event_Id_00) ;
	else
		Event_post(edgeDetectEvent, Event_Id_01) ;
}

Int32 fxnTest1(UInt32 size, UInt32 *data)
{
    FxnArgs *args = (FxnArgs *)((UInt32)data + sizeof(map_info_type));
    t1.buffer1 = t2.buffer1 = (int(*)[SIZE])(args->a) ;
    t1.buffer2 = t2.buffer2 = (int(*)[SIZE])(args->b) ; 
    t1.buffer3 = t2.buffer3 = (int(*)[SIZE])(args->c) ;
    t1.start_indx = t2.start_indx = args->start_indx;
    t1.end_indx = t2.end_indx = args->end_indx;

    int num_bytes = (&(t1.buffer1[t2.end_indx-1][SIZE-1]) - &(t1.buffer1[t1.start_indx][0]) + 1) * sizeof(int) ;

    invalidate_caches(t1.buffer1, t1.buffer2, t1.buffer3, num_bytes) ;

    int unit_work = (t2.end_indx - t1.start_indx)/2 ;
    t1.end_indx = t1.start_indx + unit_work ;
    t2.start_indx = t2.start_indx + unit_work ;
    
    task0 = Task_create((Task_FuncPtr)common_wrapper, &taskParams0, &eb0);
    if (task0 == NULL) {
	    System_abort("Task create failed");
    }   

    task1 = Task_create((Task_FuncPtr)common_wrapper, &taskParams1, &eb1);
    if (task1 == NULL) {
      //    System_abort("Task create failed");
    }

    UInt events ;
    events = Event_pend(edgeDetectEvent, Event_Id_00 + Event_Id_01, Event_Id_NONE, BIOS_WAIT_FOREVER) ;

    Task_delete(&task0) ;
    Task_delete(&task1) ;

    wb_invalidate_caches(&(t1.buffer1[t1.start_indx][0]), &(t1.buffer2[t1.start_indx][0]), 
			     &(t1.buffer3[t1.start_indx][0]), num_bytes) ;

    Uint32 reta = 1 ;
    return reta ;
}


Int main(Int argc, char* argv[])
{
    RcmServer_Params  rcmServerParams;

    System_printf("%s starting..\n", MultiProc_getName(MultiProc_self()));

    /*
     * Enable use of runtime Diags_setMask per module:
     *
     * Codes: E = ENTRY, X = EXIT, L = LIFECYCLE, F = INFO, S = STATUS
     */
    Diags_setMask("ti.ipc.rpmsg.MessageQCopy=EXLFS");

    /* Setup the table of services, so clients can create and connect to
     * new service instances:
     */
    ServiceMgr_init();

    /* initialize RcmServer create params */
    RcmServer_Params_init(&rcmServerParams);

    /* The first function, at index 0, is a special create function, which
     * gets passed a Service_Handle argument.
     * We set this at run time as our C compiler is not allowing named union
     * field initialization:
     */
    OMXServer_fxnTab.elem[0].addr.createFxn = RPC_SKEL_GetHandle;

    rcmServerParams.priority    = Thread_Priority_ABOVE_NORMAL;
    rcmServerParams.fxns.length = OMXServer_fxnTab.length;
    rcmServerParams.fxns.elem   = OMXServer_fxnTab.elem;

    /* Register an OMX service to create and call new OMX components: */
    ServiceMgr_register("OMX", &rcmServerParams);

    /* Some background ping testing tasks, used by rpmsg samples: */
    start_ping_tasks();

#if 0 /* DSP or CORE0 or IPU */
    /* Run a background task to test rpmsg_resmgr service */
    start_resmgr_task();
#endif

#if 0  /* DSP or CORE0 or IPU */
    /* Run a background task to test hwspinlock */
    start_hwSpinlock_task();
#endif

    /* Create 1 task with priority 15 */
    Error_init(&eb0);
    Task_Params_init(&taskParams0);
    //taskParams0.stackSize = 512;
    taskParams0.priority = 15; 
    taskParams0.affinity = 1; 
    taskParams0.arg0 = (xdc_UArg)(&(t1)) ;
    taskParams0.arg1 = 0 ;

    /* Create 1 task with priority 15 */
    Error_init(&eb1);
    Task_Params_init(&taskParams1);
    //taskParams1.stackSize = 512;
    taskParams1.priority = 15; 
    taskParams1.affinity = 0;
    taskParams1.arg0 = (xdc_UArg)(&(t2)) ;
    taskParams1.arg1 = 1 ;

    /* create an Event object. All events are binary */
    Error_Block eb;
    Error_init(&eb);
    edgeDetectEvent = Event_create(NULL, &eb);
    if (edgeDetectEvent == NULL) {
	    System_abort("Event create failed");
    }

    /* Start the ServiceMgr services */
    ServiceMgr_start(0);

    BIOS_start();

    return (0);
}
