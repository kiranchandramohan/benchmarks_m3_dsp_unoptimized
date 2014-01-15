#include "dsp.c"
#define SIZE 1024
#define NUM_ITER 1

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

Int32 fxnTest1(UInt32 size, UInt32 *data)
{
    UInt32 start_indx, end_indx ;

#if CHATTER
    System_printf("fxnInit : Executing fxnTest1\n");
#endif

    FxnArgs *args = (FxnArgs *)((UInt32)data + sizeof(map_info_type));
    int* buffer1 = (int*)args->a ;
    int* buffer2 = (int*)args->b ;
    int* buffer3 = (int*)args->c ;
    start_indx = args->start_indx;
    end_indx = args->end_indx;
    
    int i ;
    for(i=0 ; i<NUM_ITER ; i++) {
	    callBarrier(0, /*lock_id=*/4) ;
	    multiply(buffer1, buffer2, buffer3, start_indx, end_indx) ;
    }

    return 1 ;
}
