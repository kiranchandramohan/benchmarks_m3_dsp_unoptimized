#include "dsp.c"
#define NQ 64
#define NR 64
#define NP 64
#define NUM_ITER 100

void doitgen(int* restrict A, int* restrict sum, int* restrict C4, int start_indx, int end_indx)
{
	int r, q, p, s ;

	for (r = 0; r < NP; r++)
		for (q = 0; q < NP; q++)
			C4[r*NP+q] = ((int) r*q) / NP ;

	#pragma MUST_ITERATE(2,64,2)
	for (r = start_indx ; r < end_indx ; r++)
		for (q = 0 ; q < NQ ; q++)
			for (p = 0; p < NP; p++)
				A[r*NQ*NP+q*NP+p] = ((int) r*q + p) / NP ;   

	#pragma MUST_ITERATE(2,64,2)
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


Int32 fxnTest1(UInt32 size, UInt32 *data)
{
	UInt32 start_indx, end_indx ;
    FxnArgs *args = (FxnArgs *)((UInt32)data + sizeof(map_info_type));
    int* buffer1 = (int*)args->a ;
    int* buffer2 = (int*)args->b ;
    int* buffer3 = (int*)args->e ;
    start_indx = args->start_indx;
    end_indx = args->end_indx;
    
    int i ;
    for(i=0 ; i<NUM_ITER ; i++) {
	    callBarrier(0, /*lock_id=*/4) ;
	    doitgen(buffer1, buffer2, buffer3, start_indx, end_indx) ;
    }

    return 1 ;
}
