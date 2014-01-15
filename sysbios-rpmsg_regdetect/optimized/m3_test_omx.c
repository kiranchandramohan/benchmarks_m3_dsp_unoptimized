#include "m3.c"

#define MAXGRID 64
#define LENGTH 2048
#define NUM_ITER 30

void parallel_kernel_reg_detect(int tid, int* restrict sum_tang, int* restrict mean, 
	int* restrict diff, int* restrict sum_diff, int* restrict tmp, int* restrict path, 
	int start_indx, int end_indx)
{
        int i, j, cnt ;

	for (i = start_indx ; i < end_indx ; i++)
		for (j = 0; j < MAXGRID; j++) {
			sum_tang[i*MAXGRID+j] = (i+1)*(j+1);
			mean[i*MAXGRID+j] = (i-j)/MAXGRID;
			path[i*MAXGRID+j] = (i*(j-1))/MAXGRID;
		}   

        for (j = start_indx ; j < end_indx ; j++)
                for (i = j; i <= MAXGRID - 1; i++)
                        for (cnt = 0; cnt <= LENGTH - 1; cnt++) {
                                diff[j*MAXGRID*LENGTH + i*LENGTH + cnt] =
                                        sum_tang[j*MAXGRID + i]; 
                        }   

        for (j = start_indx ; j < end_indx ; j++)
        {   
                for (i = j; i <= MAXGRID - 1; i++)
                {   
                        sum_diff[j*MAXGRID*LENGTH + i*LENGTH + 0] = diff[j*MAXGRID*LENGTH + i*LENGTH + 0] ;
                        for (cnt = 1; cnt <= LENGTH - 1; cnt++) {
                                sum_diff[j*MAXGRID*LENGTH + i*LENGTH + cnt] = 
                                        sum_diff[j*MAXGRID*LENGTH + i*LENGTH + cnt-1] + 
                                        diff[j*MAXGRID*LENGTH + i*LENGTH + cnt];
                        }   

                        mean[j*MAXGRID+i] = sum_diff[j*MAXGRID*LENGTH + i*LENGTH + LENGTH-1];
                }   
        }   

	if(tid == 0) {
		callBarrier(1, /*lock_id=*/4) ;
	}
	callLocalBarrier() ;

	int x ;
	for (j = start_indx ; j < end_indx ; j++) {
		x = 0 ;
		tmp[j*MAXGRID+j] = mean[x*MAXGRID+j] ;
		//printf("tmp[%d][%d] = %d\n", j, j, tmp[j*MAXGRID+j]) ;
		for (i = j+1; i <= MAXGRID - 1; i++) {
			x++ ;
			tmp[j*MAXGRID+i] = tmp[j*MAXGRID+i-1] + mean[x*MAXGRID+i];
			//printf("tmp[%d][%d] = %d\n", j, i, tmp[j*MAXGRID+i]) ;
		}
	}
	
	if(tid == 0) {
		callBarrier(2, /*lock_id=*/4) ;
	}
	callLocalBarrier() ;

	//print_array((int*)tmp) ;

	for (j = start_indx ; j < end_indx ; j++) {
		x = 0 ;
		for (i = j ; i <= MAXGRID - 1; i++) {
			path[j*MAXGRID+i] = tmp[x*MAXGRID+i] ;
			x++ ;
		}
	}
	//print_array((int*)path) ;
}

void common_wrapper(UArg arg0, UArg arg1)
{
	taskArgs* t = (taskArgs*)arg0 ;
	int tid = (int)arg1 ;

	int* sum_tang = t->buffer1 ;
	int* mean = t->buffer2 ;
	int* diff = t->buffer3 ;
	int* sum_diff = t->buffer4 ;
	int* tmp = t->buffer5 ;
	int* path = t->buffer6 ;
	int start_indx = t->start_indx ;
	int end_indx = t->end_indx ;

	int i ;
	for(i=0 ; i<NUM_ITER ; i++) {
		if(tid == 0) {
			callBarrier(0, /*lock_id=*/4) ;
		}
		callLocalBarrier() ;
		parallel_kernel_reg_detect(tid, sum_tang, mean, diff, sum_diff, 
					tmp, path, start_indx, end_indx) ;
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
    t1.buffer4 = t2.buffer4 = (int*)(args->d) ;
    t1.buffer5 = t2.buffer5 = (int*)(args->e) ;
    t1.buffer6 = t2.buffer6 = (int*)(args->f) ;
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

