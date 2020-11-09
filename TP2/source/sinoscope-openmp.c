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

    sinoscope_t sinos = *sinoscope;
    unsigned char* buffer = sinos.buffer;
    
    #pragma omp parallel for  \
    schedule(static) private(i,j,k,index,px,py,pixel,value) collapse(2) \
    default(none) shared(sinos,buffer)
    for (j = 0; j < sinos.height; j++) { 
        for (i = 0; i < sinos.width; i++) {
            px    = sinos.dx * j - 2 * M_PI;
            py    = sinos.dy * i - 2 * M_PI;
            value = 0.0;
            
            for (k = 1; k <= sinos.taylor; k += 2) {
                value += sin(px * k * sinos.phase1 + sinos.time) / k + cos(py * k * sinos.phase0) / k;
            }


            value = (atan(value) - atan(-value)) / M_PI;
            value = (value + 1) * 100;
            
            color_value(&pixel, value, sinos.interval, sinos.interval_inverse);
            index = (i * 3) + (j * 3) * sinos.width;
            
            buffer[index + 0] = pixel.bytes[0]; 
            buffer[index + 1] = pixel.bytes[1];
            buffer[index + 2] = pixel.bytes[2];
        }

    }

    /***************DO NO FORGET TO FREE THE MEMORY ***/

    return 0;

    fail_exit:
        return -1;
    
}
