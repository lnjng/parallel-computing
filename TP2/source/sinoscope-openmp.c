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
    float px;
    float py;
    float value;
    int index;
    pixel_t pixel;

 
    
   
    #pragma omp parallel for schedule(static) private(i,j,k,px,py,value,index,pixel)
    for (i = 0; i < sinoscope->width; i++) {
     
       #pragma omp parallel for schedule(static) 
        for (j = 0; j < sinoscope->height; j++) {
            px    = sinoscope->dx * j - 2 * M_PI;
            py    = sinoscope->dy * i - 2 * M_PI;
            value = 0;
 
            #pragma omp parallel for simd schedule(static) 
            for (k = 1; k <= sinoscope->taylor; k += 2) {
                value += sin(px * k * sinoscope->phase1 + sinoscope->time) / k;
                value += cos(py * k * sinoscope->phase0) / k;
            }
            
            #pragma omp atomic write
            value = (atan(value) - atan(-value)) / M_PI;
            value = (value + 1) * 100;

            
            color_value(&pixel, value, sinoscope->interval, sinoscope->interval_inverse);


            #pragma omp atomic write
            index = (i * 3) + (j * 3) * sinoscope->width;
            sinoscope->buffer[index + 0] = pixel.bytes[0];
            sinoscope->buffer[index + 1] = pixel.bytes[1];
            sinoscope->buffer[index + 2] = pixel.bytes[2]; 
            }

        }

    return 0;

    fail_exit:
        return -1;
    
}
