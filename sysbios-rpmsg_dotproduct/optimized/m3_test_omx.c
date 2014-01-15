#include "m3.c"
#define NUM_ITER 100
#define SIZE 2048

void dot_product(int* restrict A, int* restrict B, int* restrict result, int start, int end)
{                                                    
        int i ;
        int sum = 0 ;
	
	for (i = start ; i < end ; i++)
	{
			A[i] = B[i] = i ;
	}

        for (i = start ; i < end ; i++)
	{
		sum += A[i] * B[i] ;
	}

	*result = sum ;

}

Int mem[2] ;
void common_wrapper(UArg arg0, UArg arg1)
{
	taskArgs* t = (taskArgs*)arg0 ;
	int tid = (int)arg1 ;
	int i ;
	volatile int* result = (int*)BARRIER_CNTR_BASE + REDUCTION_OFFSET ;
	
	for(i=0 ; i<NUM_ITER ; i++) {
		if(tid == 0) {
			callBarrier(0, /*lock_id=*/4) ;
			mem[0] = mem[1] = 0 ;
			result[0] = 0 ;
			callBarrier(1, /*lock_id=*/4) ;
			callLocalBarrier() ;
			dot_product(t->buffer1, t->buffer2, &(mem[0]), t->start_indx, t->end_indx) ;
			callLocalBarrier() ;
			HwSpinlock_Handle handle ;
			HwSpinlock_Key key ;
			lock(&handle, &key, /*id=*/5) ;
			result[0] += mem[0] + mem[1] ;
			unlock(&handle, &key) ;
			callBarrier(2, /*lock_id=*/4) ;
		} else {
			callLocalBarrier() ;
			dot_product(t->buffer1, t->buffer2, &(mem[1]), t->start_indx, t->end_indx) ;
			callLocalBarrier() ;
		}
		
	}

	if(tid == 0)
		Event_post(edgeDetectEvent, Event_Id_00) ;
	else
		Event_post(edgeDetectEvent, Event_Id_01) ;
}

Int32 fxnTest1(UInt32 size, UInt32 *data)
{
    FxnArgs *args = (FxnArgs *)((UInt32)data + sizeof(map_info_type));
    t1.buffer1 = t2.buffer1 = (int*)(args->a) ;
    t1.buffer2 = t2.buffer2 = (int*)(args->b) ; 
    t1.buffer3 = t2.buffer3 = (int*)(args->c) ;
    t1.start_indx = t2.start_indx = args->start_indx;
    t1.end_indx = t2.end_indx = args->end_indx;

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


    Uint32 reta = 1 ;
    return reta ;
}
