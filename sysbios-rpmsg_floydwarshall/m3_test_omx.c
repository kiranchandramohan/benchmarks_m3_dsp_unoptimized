#include "m3.c"
#define NUM_ITER 10
#define N 512

inline int mod(int n)
{
	if(n<0)
		return -n ;
	else
		return n ;
}

void floyd_warshall(int tid, int* path, int start_indx, int end_indx)
{
	int i, j, k ;
	for(i=start_indx ; i<end_indx ; i++)
		for(j=0 ; j<N ; j++)
			path[i*N+j] = mod(i-j);                        

	for (k = 0; k < N; k++)
	{
		if(tid == 0) {
			callBarrier(1, /*lock_id=*/4) ;
			callBarrier(2, /*lock_id=*/4) ;
		}
		callLocalBarrier() ;
		for(i = start_indx ; i < end_indx ; i++)
			for (j = 0; j < N; j++)
				path[i*N+j] = path[i*N+j] < path[i*N+k] + path[k*N+j] ? 
						path[i*N+j] : path[i*N+k] + path[k*N+j] ;
	}
}

void common_wrapper(UArg arg0, UArg arg1)
{
	taskArgs* t = (taskArgs*)arg0 ;
	int tid = (int)arg1 ;
	int* path = (int*)t->buffer1 ;
	int start_indx = t->start_indx ;
	int end_indx = t->end_indx ;

	int i ;
	for(i=0 ; i<NUM_ITER ; i++) {
		if(tid == 0) {
			callBarrier(0, /*lock_id=*/4) ;
		}
		callLocalBarrier() ;
		floyd_warshall(tid, path, start_indx, end_indx) ;
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
