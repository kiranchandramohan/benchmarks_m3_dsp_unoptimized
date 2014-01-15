#include "dsp.c"
#define N 512
#define NUM_ITER 10

inline int mod(int n)
{
	if(n<0)
		return -n ;
	else
		return n ;
}

void floyd_warshall(int* restrict path, int start_indx, int end_indx)
{
	int i, j, k ;
	for(i=start_indx ; i<end_indx ; i++)
		for(j=0 ; j<N ; j++)
			path[i*N+j] = mod(i-j);                        

	for (k = 0; k < N; k++)
	{
		callBarrier(1, /*lock_id=*/4) ;
		callBarrier(2, /*lock_id=*/4) ;
		#pragma MUST_ITERATE(16,512,16)
		for(i = start_indx ; i < end_indx ; i++)
			for (j = 0; j < N; j++)
				path[i*N+j] = path[i*N+j] < path[i*N+k] + path[k*N+j] ? 
						path[i*N+j] : path[i*N+k] + path[k*N+j] ;
	}
}

Int32 fxnTest1(UInt32 size, UInt32 *data)
{
    UInt32 start_indx, end_indx ;

#if CHATTER
    System_printf("fxnInit : Executing fxnTest1\n");
#endif

    FxnArgs *args = (FxnArgs *)((UInt32)data + sizeof(map_info_type));
    int* path = (int*)args->a ;
    start_indx = args->start_indx ;
    end_indx = args->end_indx ;
    
    int i ;
    for(i=0 ; i<NUM_ITER ; i++) {
	    callBarrier(0, /*lock_id=*/4) ;
	    floyd_warshall(path, start_indx, end_indx) ;
    }

    return 1 ;
}
