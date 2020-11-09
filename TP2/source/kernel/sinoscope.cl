#include "helpers.cl"

/*
 * TODO
 *
 * Déclarez les structures partagées avec l'autre code si nécessaire selon votre énoncé.
 * Utilisez l'attribut `__attribute__((packed))` à vos déclarations.
 */
struct __attribute__((packed)) params_int_t {
    unsigned int buffer_size;
    unsigned int width;
    unsigned int height;
    unsigned int taylor;
    unsigned int interval;
};
struct __attribute__((packed)) params_float_t {
    float interval_inverse;
    float time;
    float max;
    float phase0;
    float phase1;
    float dx;
    float dy;
};
/*
 * TODO
 *
 * Modifiez les paramètres du noyau avec ceux demandés par votre énoncé.
 */

//__kernel void sinoscope_kernel(__global unsigned char* buffer, struct params_int_t sinoscope_ints, struct params_float_t sinoscope_floats) {
__kernel void sinoscope_kernel(__global unsigned char* buffer, struct params_int_t sinoscope_ints, float interval_inverse, 
                                float time, float max, float phase0, float phase1, float dx, float dy) 
{
    /*
     * TODO
     *
     * En vous basant sur l'implémentation dans `sinoscope-serial.c`, effectuez le même calcul. Ce
     * noyau est appelé pour chaque coordonnée, alors vous ne devez pas spécifiez les boucles
     * extérieures.
     */

    int i, j;
	
	i = get_global_id(0);
	j = get_global_id(1);

    float px    = dx * j - 2 * M_PI;
    float py    = dy * i - 2 * M_PI;
    float value = 0;

    for (int k = 1; k <= sinoscope_ints.taylor; k += 2) {
        value += sin(px * k * phase1 + time) / k;
        value += cos(py * k * phase0) / k;
    }

    value = (atan(value) - atan(-value)) / M_PI;
    value = (value + 1) * 100;

    pixel_t pixel;
    color_value(&pixel, value, sinoscope_ints.interval, interval_inverse);

    int index = (i * 3) + (j * 3) * sinoscope_ints.width;

    buffer[index + 0] = pixel.bytes[0];
    buffer[index + 1] = pixel.bytes[1];
    buffer[index + 2] = pixel.bytes[2];
}
