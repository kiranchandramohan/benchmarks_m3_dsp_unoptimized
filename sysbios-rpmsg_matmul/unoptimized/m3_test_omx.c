#include "m3.c"
#define NUM_ITER 1
#define SIZE 1024

void multiply(int* A, int* B, int* C, int start, int end)
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

void common_wrapper(UArg arg0, UArg arg1)
{
	taskArgs* t = (taskArgs*)arg0 ;
	int tid = (int)arg1 ;
	int i ;
	for(i=0 ; i<NUM_ITER ; i++) {
		if(tid == 0) {
			callBarrier(0, /*lock_id=*/4) ;
			callLocalBarrier() ;
			multiply(t->buffer1, t->buffer2, t->buffer3, t->start_indx, t->end_indx) ;
		} else {
			callLocalBarrier() ;
			multiply(t->buffer1, t->buffer2, t->buffer3, t->start_indx, t->end_indx) ;
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
