#include "m3.c"
#define NUM_ITER 30
#define IMG_SIZE 4096
#define HISTO_SIZE 256
#define NUM_TOTAL_THREADS 5
int* histo[NUM_TOTAL_THREADS] ;
int m3_histo[HISTO_SIZE] ;
int m3_gray_level_mapping[HISTO_SIZE] ;

void* compute_histo(int* image, int* histo, int img_start_indx, int img_end_indx) 
{
        int i, j ;
	
	for (i = 0 ; i < HISTO_SIZE ; i++) {
		histo[0*HISTO_SIZE+i] = 0;
	}  

        for (i = img_start_indx ; i < img_end_indx ; i++) {
                for (j = 0; j < IMG_SIZE; ++j) {
                        image[i*IMG_SIZE+j] = (i*j) % 255 ;
                }   
        }   

        /* Compute the image's histogram */
        for (i = img_start_indx ; i < img_end_indx ; i++) {
                for (j = 0; j < IMG_SIZE; ++j) {
                        int pix = image[i*IMG_SIZE+j] ;
                        histo[0*HISTO_SIZE+pix] += 1;
                }   
        }   

        return NULL ;
}

void* compute_image(int* image, int* gray_level_mapping, int img_start_indx, int img_end_indx) 
{
        int i, j ; 

        /* Map the old gray levels in the original image to the new gray levels. */
        for (i = img_start_indx ; i < img_end_indx ; i++) {
                for (j = 0; j < IMG_SIZE; ++j) {
                        image[i*IMG_SIZE+j] = gray_level_mapping[image[i*IMG_SIZE+j]];
                }   
        }   

        return NULL ;
}

void compute_gray_level_mapping()
{
	int j, k ;
	float cdf, pixels ;

	for(j=0 ; j<NUM_TOTAL_THREADS ; j++) {
		for(k=0 ; k<HISTO_SIZE ; k++) {
			if(j==0)
				m3_histo[k] = (histo[j])[k] ;
			else
				m3_histo[k] += (histo[j])[k] ;
		}
	}

	cdf = 0.0 ;
	pixels = (float)(IMG_SIZE * IMG_SIZE) ;
	for(j=0 ; j<HISTO_SIZE ; j++) {
		cdf += ((float)m3_histo[j])/pixels ;
		m3_gray_level_mapping[j] = (int)(255.0 * cdf) ;
	}
}

void common_wrapper(UArg arg0, UArg arg1)
{
	taskArgs* t = (taskArgs*)arg0 ;
	int tid = (int)arg1 ;
	int i, j, k ;
	
	for(i=0 ; i<NUM_ITER ; i++) {
		if(tid == 0) {
			callBarrier(0, /*lock_id=*/4) ;
		}

		callLocalBarrier() ;
		compute_histo(t->buffer1, t->buffer2, t->start_indx, t->end_indx) ;
		Cache_wbInv (t->buffer2, HISTO_SIZE*4, Cache_Type_ALL, FALSE) ;
		callLocalBarrier() ;
		if(tid == 0) {
			callBarrier(1, /*lock_id=*/4) ;
			compute_gray_level_mapping() ;
		}
		callLocalBarrier() ;
		compute_image(t->buffer1, m3_gray_level_mapping, t->start_indx, t->end_indx) ;

		if(tid == 0)
			Cache_wbInv (t->buffer1, IMG_SIZE*IMG_SIZE*4, Cache_Type_ALL, FALSE);
	}

	if(tid == 0)
		Event_post(edgeDetectEvent, Event_Id_00) ;
	else
		Event_post(edgeDetectEvent, Event_Id_01) ;
}

Int32 fxnTest1(UInt32 size, UInt32 *data)
{
    FxnArgs *args = (FxnArgs *)((UInt32)data + sizeof(map_info_type));
    t1.buffer1 = t2.buffer1 = (int*)args->a ;
    histo[0] = (int*)args->b ;
    histo[1] = (int*)args->c ;
    histo[2] = (int*)args->d ;
    histo[3] = (int*)args->e ;
    histo[4] = (int*)args->f ;
    t1.buffer2 = histo[2] ; 
    t2.buffer2 = histo[3] ;
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


    return 1 ;
}
