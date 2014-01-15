#include "m3.c"

/* Turn on/off printf's */
#define NQ 64
#define NR 64
#define NP 64
#define NUM_ITER 100

void doitgen(int* A, int* sum, int* C4, int start_indx, int end_indx)
{
	int r, q, p, s ;

	for (r = 0; r < NP; r++)
		for (q = 0; q < NP; q++)
			C4[r*NP+q] = ((int) r*q) / NP ;

	for (r = start_indx ; r < end_indx ; r++)
		for (q = 0 ; q < NQ ; q++)
			for (p = 0; p < NP; p++)
				A[r*NQ*NP+q*NP+p] = ((int) r*q + p) / NP ;   

	for (r = start_indx ; r < end_indx ; r++) {
		for (q = 0; q < NQ; q++) {
			for (p = 0; p < NP; p++) {                                                
				sum[r*NQ*NP+q*NP+p] = 0;                                  
				for (s = 0; s < NP; s++)  {
					sum[r*NQ*NP+q*NP+p] = sum[r*NQ*NP+q*NP+p] + 
							A[r*NQ*NP+q*NP+s] * C4[s*NP+p];       
				}
			}

			for (p = 0; p < NR; p++) {
				A[r*NQ*NP+q*NP+p] = sum[r*NQ*NP+q*NP+p] ;
			}
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
			doitgen(t->buffer1, t->buffer2, t->buffer3, t->start_indx, t->end_indx) ;
		} else {
			callLocalBarrier() ;
			doitgen(t->buffer1, t->buffer2, t->buffer3, t->start_indx, t->end_indx) ;
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
    t1.buffer3 = (int*)(args->c) ; 
    t2.buffer3 = (int*)(args->d) ; 
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
