#include "dsp.c"
#define NUM_ITER 1
#define IMG_SIZE 4096
#define HISTO_SIZE 256
#define NUM_TOTAL_THREADS 5
int* histo[NUM_TOTAL_THREADS] ;
int dsp_histo[HISTO_SIZE] ;
int dsp_gray_level_mapping[HISTO_SIZE] ;

void* compute_histo(int* restrict image, int* restrict histo, int img_start_indx, int img_end_indx) 
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

void* compute_image(int* restrict image, int* restrict gray_level_mapping, int img_start_indx, int img_end_indx) 
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
	int cdf, pixels ;

	for(j=0 ; j<NUM_TOTAL_THREADS ; j++) {
		for(k=0 ; k<HISTO_SIZE ; k++) {
			if(j==0)
				dsp_histo[k] = (histo[j])[k] ;
			else
				dsp_histo[k] += (histo[j])[k] ;
		}
	}
	cdf = 0 ;
	pixels = (int)(IMG_SIZE * IMG_SIZE) ;
	for(j=0 ; j<HISTO_SIZE ; j++) {
		cdf += ((int)(dsp_histo[j]))/pixels ;
		dsp_gray_level_mapping[j] = (int)(255 * cdf) ;
	}
}

Int32 fxnTest1(UInt32 size, UInt32 *data)
{
    UInt32 start_indx,end_indx;
    FxnArgs *args = (FxnArgs *)((UInt32)data + sizeof(map_info_type));
    start_indx = args->start_indx;
    end_indx = args->end_indx;

#if CHATTER
    System_printf("fxnInit : Executing fxnTest1\n");
#endif
	
    int* image = (int*)(args->a) ;
    int* tmp_histo = (int*)(args->f) ; 
    histo[0] = (int*)(args->b) ;
    histo[1] = (int*)(args->c) ;
    histo[2] = (int*)(args->d) ;
    histo[3] = (int*)(args->e) ;
    histo[4] = (int*)(args->f) ;

    int i ;
    for(i=0 ; i<NUM_ITER ; i++) {
	    callBarrier(0, /*lock_id=*/4) ;
	    compute_histo(image, tmp_histo, start_indx, end_indx) ;
	    Cache_wbInv (tmp_histo, HISTO_SIZE*4, Cache_Type_ALL, FALSE);
	    callBarrier(1, /*lock_id=*/4) ;
	    compute_gray_level_mapping() ;
	    compute_image(image, dsp_gray_level_mapping, start_indx, end_indx) ;
	    Cache_wbInv (image, IMG_SIZE*IMG_SIZE*4, Cache_Type_ALL, FALSE);
    }

    return 1 ;
}
