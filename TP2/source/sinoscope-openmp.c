#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "color.h"
#include "log.h"
#include "sinoscope.h"

int sinoscope_image_openmp(sinoscope_t* sinoscope) {
    /*
     * TODO
     *
     * En vous basant sur l'implémentation dans `sinoscope-serial.c`, effectuez le même calcul en
     * utilisant les pragmas OpenMP selon votre énoncé.
     */

    if (sinoscope == NULL) {
        LOG_ERROR_NULL_PTR();
        goto fail_exit;
    }

    int i;
    int j;
    int k;
    pixel_t pixel;
    int index;
   
    

    #pragma omp parallel for schedule(static,10) private(i,j,k,pixel,index) collapse(2)
    for (i = 0; i < sinoscope->width; i++) { 
        for (j = 0; j < sinoscope->height; j++) {
            float px    = sinoscope->dx * j - 2 * M_PI;
            float py    = sinoscope->dy * i - 2 * M_PI;
            float value = 0.0;
            
            #pragma omp simd reduction(+: value) 
            for (k = 1; k <= sinoscope->taylor; k += 2) {
                value += sin(px * k * sinoscope->phase1 + sinoscope->time) / k;
                value += cos(py * k * sinoscope->phase0) / k;
            }


            value = (atan(value) - atan(-value)) / M_PI;
            value = (value + 1) * 100;
            
            color_value(&pixel, value, sinoscope->interval, sinoscope->interval_inverse);
            index = (i * 3) + (j * 3) * sinoscope->width;

            #pragma omp critical(section1)
            {
                sinoscope->buffer[index + 0] = pixel.bytes[0];
                sinoscope->buffer[index + 1] = pixel.bytes[1];
                sinoscope->buffer[index + 2] = pixel.bytes[2];
            }
            
        }

    }
    return 0;

    fail_exit:
        return -1;
    
}
