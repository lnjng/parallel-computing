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
    double* values = malloc(sizeof(double) * (sinoscope->buffer_size));

    
    #pragma omp parallel for schedule(static) private(i,j,k,index,px,py,pixel,value) collapse(2)
    for (i = 0; i < sinos.width; i++) { 
        for (j = 0; j < sinos.height; j++) {
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
            values[index] = value;
            

            /*buffer[index + 0] = pixel.bytes[0]; 
            buffer[index + 1] = pixel.bytes[1]; 
            buffer[index + 2] = pixel.bytes[2];*/
            
        }

    }

    //#pragma omp parallel for schedule(static) private(i,j,index) firstprivate(buffer) collapse(2) ordered
    for (i = 0; i < sinos.width; i++)
    {

        for (j = 0;  j < sinos.height; j++)
        {
            index = (i * 3) + (j * 3) * sinos.width;
            pixel_t pixel;
            color_value(&pixel, values[index], sinos.interval, sinos.interval_inverse);
            //#pragma omp ordered
            //{
                buffer[index + 0] = pixel.bytes[0]; 
                buffer[index + 1] = pixel.bytes[1]; 
                buffer[index + 2] = pixel.bytes[2];   
            //}
        }
    }

    return 0;

    fail_exit:
        return -1;
    
}
