#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>

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

    int i,j,k, index;
    float px, py,value = 0.0;
    pixel_t pixel;
    
    #pragma omp parallel for  \
    schedule(static) private(i,j,k,index,px,py,pixel,value) collapse(2) \
    default(none) shared(sinoscope)
    for (i = 0; i < sinoscope->width; i++) { 
        for (j = 0; j < sinoscope->height; j++) {
            px    = sinoscope->dx * j - 2 * M_PI;
            py    = sinoscope->dy * i - 2 * M_PI;
            value = 0.0;
            
            for (k = 1; k <= sinoscope->taylor; k += 2) {
                value += sin(px * k * sinoscope->phase1 + sinoscope->time) / k;
                value += cos(py * k * sinoscope->phase0) / k;
            }


            value = (atan(value) - atan(-value)) / M_PI;
            value = (value + 1) * 100;
            
            color_value(&pixel, value, sinoscope->interval, sinoscope->interval_inverse);
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
