#include "m3.c"
#define SIZE 2048
//#define SIZE 8
#define         K       3
#define         N       SIZE
#define         T       127

#define GAUSSIAN 1
#define VERTICAL_SOBEL 2
#define HORIZONTAL_SOBEL 3
#define CONVOLVE 4
#define THRESHOLD 5
#define INIT 6
#define NUM_ITER 10

int filter[K][K] ;

void wb_invalidate_caches(void *ib1, void* ib2, void* ib3, int num_bytes)
{
	Cache_wbInv (ib1, num_bytes, Cache_Type_ALL, FALSE);
	Cache_wbInv (ib2, num_bytes, Cache_Type_ALL, FALSE);
	Cache_wbInv (ib3, num_bytes, Cache_Type_ALL, FALSE);
}

void invalidate_caches(void *ib1, void* ib2, void* ib3, int num_bytes)
{
	Cache_inv (ib1, SIZE*SIZE*4, Cache_Type_ALL, FALSE) ;
	Cache_inv (ib2, SIZE*SIZE*4, Cache_Type_ALL, FALSE) ;
	Cache_inv (ib3, SIZE*SIZE*4, Cache_Type_ALL, FALSE);
}

int abs(int n)
{
        return (n<0) ? (-n) : n ;
}

void initialize(int* image_buffer1, int* image_buffer2, int* image_buffer3,
                int start_indx, int end_indx)
{
  int i, j ;

  /* Read input image. */
  //input_dsp(image_buffer1, N*N, 1);

  /* Initialize image_buffer2 and image_buffer3 */
  for (i = start_indx; i < end_indx; i++) {
    for (j = 0; j < N; ++j) {
       image_buffer1[i*N+j] = i+j ;

       image_buffer2[i*N+j] = 0;

       image_buffer3[i*N+j] = 0;
     }
  }
}

/* This function convolves the input image by the kernel and stores the result
   in the output image. */
void convolve2d(int* input_image, int kernel[K][K], int* output_image,
                int start_indx, int end_indx)
{
  int i;
  int j;
  int c;
  int r;
  int normal_factor;
  int sum;
  int dead_rows;
  int dead_cols;

  /* Set the number of dead rows and columns. These represent the band of rows
     and columns around the edge of the image whose pixels must be formed from
     less than a full kernel-sized compliment of input image pixels. No output
     values for these dead rows and columns since  they would tend to have less
     than full amplitude values and would exhibit a "washed-out" look known as
     convolution edge effects. */

  dead_rows = K / 2;
  dead_cols = K / 2;

  /* Calculate the normalization factor of the kernel matrix. */

  normal_factor = 0;
  for (r = 0; r < K; r++) {
    for (c = 0; c < K; c++) {
      normal_factor += abs(kernel[r][c]);
    }
  }

  if (normal_factor == 0)
    normal_factor = 1;

  /* Convolve the input image with the kernel. */
  for (r = start_indx; r < end_indx - K + 1; r++) {
    for (c = 0; c < N - K + 1; c++) {
      sum = 0;
      for (i = 0; i < K; i++) {
        for (j = 0; j < K; j++) {
          sum += input_image[(r+i)*N+c+j] * kernel[i][j];
        }
      }
      output_image[(r+dead_rows)*N+c+dead_cols] = (sum / normal_factor);
    }
  }
}

void set_filter(int filter[K][K], int type)
{
        if(type == GAUSSIAN) {
                filter[0][0] = 1;
                filter[0][1] = 2;
                filter[0][2] = 1;
                filter[1][0] = 2;
                filter[1][1] = 4;
                filter[1][2] = 2;
                filter[2][0] = 1;
                filter[2][1] = 2;
                filter[2][2] = 1;
        } else if(type == VERTICAL_SOBEL) {
                filter[0][0] =  1;  
                filter[0][1] =  0;  
                filter[0][2] = -1; 
                filter[1][0] =  2;  
                filter[1][1] =  0;  
                filter[1][2] = -2; 
                filter[2][0] =  1;  
                filter[2][1] =  0;  
                filter[2][2] = -1; 
        } else if(type == HORIZONTAL_SOBEL) {
                filter[0][0] =  1;  
                filter[0][1] =  2;  
                filter[0][2] =  1;  
                filter[1][0] =  0;  
                filter[1][1] =  0;  
                filter[1][2] =  0;  
                filter[2][0] = -1; 
                filter[2][1] = -2; 
                filter[2][2] = -1; 
        }   
}

void apply_threshold(int* input_image1, int* input_image2, int* output_image,
        int start_indx, int end_indx)
{
  /* Take the larger of the magnitudes of the horizontal and vertical
     matrices. Form a binary image by comparing to a threshold and
     storing one of two values. */
  int i, j ; 
  int temp1, temp2, temp3 ;

  for (i = start_indx; i < end_indx; i++) {
    for (j = 0; j < N; ++j) {
       temp1 = abs(input_image1[i*N+j]);
       temp2 = abs(input_image2[i*N+j]);
       temp3 = (temp1 > temp2) ? temp1 : temp2;
       output_image[i*N+j] = (temp3 > T) ? 255 : 0;
     }   
  }
}

inline int get_start_indx(taskArgs* t)
{
	return (t->start_indx != 0) ? t->start_indx - 1 : t->start_indx ;
}

inline int get_end_indx(taskArgs* t)
{
	return (t->end_indx != SIZE) ? t->end_indx + 1 : t->end_indx ;
}

void common_wrapper(UArg arg0, UArg arg1)
{
	int filter[K][K] ;
	taskArgs* t = (taskArgs*)arg0 ;
	int tid = (int)arg1 ;

	int start_indx = get_start_indx(t) ;
	int end_indx = get_end_indx(t) ;
	//System_printf("After Adjustment : Handling from %d to %d\n",start_indx,end_indx) ;

	int i ;
	for(i=0 ; i<NUM_ITER ; i++)
	{
		initialize(t->buffer1, t->buffer2, t->buffer3, t->start_indx, t->end_indx) ;
		callLocalBarrier() ;
		if(tid == 0)
			callBarrier(0,/*lock_id=*/4) ;

		//System_printf("Handling from %d to %d\n",t->start_indx,t->end_indx) ;

		/* Set the values of the filter matrix to a Gaussian kernel.
		   This is used as a low-pass filter which blurs the image so as to
		   de-emphasize the response of some isolated points to the edge
		   detection (Sobel) kernels. */
		set_filter(filter, GAUSSIAN) ;
		convolve2d(t->buffer1, filter, t->buffer3, start_indx, end_indx);
		callLocalBarrier() ;
		if(tid == 0)
			callBarrier(1,/*lock_id=*/4) ;

		/* Set the values of the filter matrix to the vertical Sobel operator. */
		set_filter(filter, VERTICAL_SOBEL) ;
		convolve2d(t->buffer3, filter, t->buffer1, start_indx, end_indx);

		/* Set the values of the filter matrix to the horizontal Sobel operator. */
		set_filter(filter, HORIZONTAL_SOBEL) ;
		convolve2d(t->buffer3, filter, t->buffer2, start_indx, end_indx);
		callLocalBarrier() ;
		if(tid == 0)
			callBarrier(2,/*lock_id=*/4) ;

		apply_threshold(t->buffer1, t->buffer2, t->buffer3, t->start_indx, t->end_indx) ;
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

    int num_bytes = (&(t1.buffer1[(t2.end_indx-1)*SIZE+SIZE-1]) - &(t1.buffer1[t1.start_indx*SIZE+0]) + 1) * sizeof(int) ;

    invalidate_caches(t1.buffer1, t1.buffer2, t1.buffer3, num_bytes) ;

    int unit_work = (t2.end_indx - t1.start_indx)/2 ;
    t1.end_indx = t1.start_indx + unit_work ;
    t2.start_indx = t2.start_indx + unit_work ;
    
    task0 = Task_create((Task_FuncPtr)common_wrapper, &taskParams0, &eb0);
    if (task0 == NULL) {
	    System_abort("Task create failed");
    }   

    //System_printf("fxnEdgeDetect : Creating second thread\n");

    task1 = Task_create((Task_FuncPtr)common_wrapper, &taskParams1, &eb1);
    if (task1 == NULL) {
      //    System_abort("Task create failed");
    }

    UInt events ;
    events = Event_pend(edgeDetectEvent, Event_Id_00 + Event_Id_01, Event_Id_NONE, BIOS_WAIT_FOREVER) ;

    Task_delete(&task0) ;
    Task_delete(&task1) ;

    wb_invalidate_caches(&(t1.buffer1[t1.start_indx*SIZE+0]), &(t1.buffer2[t1.start_indx*SIZE+0]), 
			     &(t1.buffer3[t1.start_indx*SIZE+0]), num_bytes) ;

    Uint32 reta = 1 ;
    return reta ;
}

