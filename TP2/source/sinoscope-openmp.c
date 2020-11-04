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
    int index;
    float px;
    float py;
    float value = 0.0;
    pixel_t pixel;
   
    
    #pragma omp parallel for schedule(static) private(i,j,k,index,px,py,pixel) collapse(2) reduction(+: value) ordered
    for (i = 0; i < sinoscope->width; i++) { 
        for (j = 0; j < sinoscope->height; j++) {
            px    = sinoscope->dx * j - 2 * M_PI;
            py    = sinoscope->dy * i - 2 * M_PI;

            #pragma omp atomic write
            value = 0.0;
            
            for (k = 1; k <= sinoscope->taylor; k += 2) {
                value += sin(px * k * sinoscope->phase1 + sinoscope->time) / k;
                value += cos(py * k * sinoscope->phase0) / k;
            }

            #pragma omp atomic write
            value = (atan(value) - atan(-value)) / M_PI;
            #pragma omp atomic write
            value = (value + 1) * 100;
               
            color_value(&pixel, value, sinoscope->interval, sinoscope->interval_inverse);
            
           
            index = (i * 3) + (j * 3) * sinoscope->width;
            
            for (int indice = 0; indice < 3; indice++)
            {
                #pragma omp atomic write
                sinoscope->buffer[index + indice] = pixel.bytes[indice];          
                
            }
            
            
        }

    }
    return 0;

    fail_exit:
        return -1;
    
}
