#include "dsp.c"

#define MAXGRID 64
#define LENGTH 2048
#define NUM_ITER 30

void parallel_kernel_reg_detect(int* sum_tang, int* mean, 
	int* diff, int* sum_diff, int* tmp, 
	int* path, int start_indx, int end_indx)
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

	callBarrier(1, /*lock_indx=*/4) ;

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
	
	callBarrier(2, /*lock_indx=*/4) ;

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


Int32 fxnTest1(UInt32 size, UInt32 *data)
{
    UInt32 start_indx, end_indx ;

#if CHATTER
    System_printf("fxnInit : Executing fxnTest1\n");
#endif

    FxnArgs *args = (FxnArgs *)((UInt32)data + sizeof(map_info_type));
    int* sum_tang = (int *)args->a ;
    int* mean = (int *)args->b ;
    int* diff = (int *)args->c ;
    int* sum_diff = (int *)args->d ;
    int* tmp = (int *)args->e ;
    int* path = (int *)args->f ;
    start_indx = args->start_indx ;
    end_indx = args->end_indx ;
    
    int i ;
    for(i=0 ; i<NUM_ITER ; i++) {
	    callBarrier(0, /*lock_id=*/4) ;
	    parallel_kernel_reg_detect(sum_tang, mean, diff, sum_diff, tmp, path, 
	    				start_indx, end_indx) ;
    }

    return 1 ;
}

