#include "dsp.c"
#define SIZE 2048
#define NUM_ITER 100

void dot_product(int* restrict A, int* restrict B, volatile int* restrict result, int start, int end)
{                                                    
        int i ;
        int sum = 0 ;
	
	#pragma MUST_ITERATE(65536,4194304,65536)
	for (i = start ; i < end ; i++)
	{
		A[i] = B[i] = i ;
	}

	#pragma MUST_ITERATE(65536,4194304,65536)
        for (i = start ; i < end ; i++)
	{
		sum += A[i] * B[i] ;
	}

        HwSpinlock_Handle handle ;
        HwSpinlock_Key key ;
	lock(&handle, &key, /*id=*/5) ;
	result[0] += sum ;
	unlock(&handle, &key) ;
}

Int32 fxnTest1(UInt32 size, UInt32 *data)
{
    UInt32 start_indx, end_indx ;

#if CHATTER
    System_printf("fxnInit : Executing fxnTest1\n");
#endif

    FxnArgs *args = (FxnArgs *)((UInt32)data + sizeof(map_info_type));
    int* buffer1 = (int*)args->a ;
    int* buffer2 = (int*)args->b ;
    start_indx = args->start_indx;
    end_indx = args->end_indx;
    
    volatile int* result = (int*)BARRIER_CNTR_BASE + REDUCTION_OFFSET ;

    int i ;
    for(i=0 ; i<NUM_ITER ; i++) {
	    callBarrier(0, /*lock_id=*/4) ;
	    result[0] = 0 ;
	    callBarrier(1, /*lock_id=*/4) ;
	    dot_product(buffer1, buffer2, result, start_indx, end_indx) ;
	    callBarrier(2, /*lock_id=*/4) ;
    }

    return 1 ;
}
