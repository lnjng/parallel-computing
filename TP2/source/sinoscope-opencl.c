#include "log.h"
#include "sinoscope.h"

int sinoscope_opencl_init(sinoscope_opencl_t* opencl, cl_device_id opencl_device_id, unsigned int width,
                          unsigned int height) {
    /*
     * TODO
     *
     * Initialiser `opencl->context` du périphérique reçu en paramètre.
     *
     * Initialiser `opencl->queue` à partir du contexte précèdent.
     *
     * Initialiser `opencl->buffer` à partir du context précèdent (width * height * 3).
     *
     * Initialiser `opencl->kernel` à partir du contexte et du périphérique reçu en
     * paramètre. Utilisez la fonction `opencl_load_kernel_code` déclaré dans `opencl.h` pour lire
     * le code du noyau OpenCL `kernel/sinoscope.cl` dans un tampon. Compiler le noyau
     * en ajoutant le paramètre `"-I " __OPENCL_INCLUDE__`.
     */

    cl_int error = 0;
    //cl_platform_id platform;

/*
    error = clGetPlatformID(&platform);
    if (error != CL_SUCCESS) {
        return -1;
    }
    error = clGetPlatformID(&platform, CL_DEVICE_TYPE_GPU, 1, &opencl_device_id, NULL);
    if (error != CL_SUCCESS) {
        return -1;
    }
*/

    opencl->context = clCreateContext(0, 1, &opencl_device_id, NULL, NULL, &error);
    if (error != CL_SUCCESS) {
        return -1;
    }

    opencl->queue = clCreateCommandQueueWithProperties(opencl->context, opencl_device_id, 0, &error);
    //opencl->queue = clCreateCommandQueue(opencl->context, opencl_device_id, 0, &error);
    if (error != CL_SUCCESS) {
        return -1;
    }

    const unsigned int buffer_size = 3 * width * height;
    opencl->buffer = clCreateBuffer(opencl->context, CL_MEM_READ_WRITE, buffer_size, NULL, &error);

    char** buffer = 0;
    size_t* length = 0;
    opencl_load_kernel_code(buffer, length);
    cl_program program = clCreateProgramWithSource(opencl->context, 1, (const char**)buffer, length, &error);
    free(buffer);
    free(length);
    if (error != CL_SUCCESS) {
        return -1;
    }

    char options[] = "\"-I \" __OPENCL_INCLUDE__";
    error = clBuildProgram(program, 1, &opencl_device_id, options, NULL, NULL);
    if (error != CL_SUCCESS) {
        return -1;
    }

    opencl->kernel = clCreateKernel(program, "sinoscope", &error);
    if (error != CL_SUCCESS) {
        return -1;
    }

    return -1;
}

void sinoscope_opencl_cleanup(sinoscope_opencl_t* opencl) {
    /*
     * TODO
     *
     * Libérez les ressources associées à `opencl->context`, `opencl->queue`,
     * `opencl->buffer` et `opencl->kernel`.
     */

    clReleaseKernel(opencl->kernel);
    clReleaseCommandQueue(opencl->queue);
    clReleaseContext(opencl->context);
}

/*
 * TODO
 *
 * Déclarez les structures partagées avec le noyau OpenCL si nécessaire selon votre énoncé.
 * Utilisez l'attribut `__attribute__((packed))` à vos déclarations.
 */

int sinoscope_image_opencl(sinoscope_t* sinoscope) {
    if (sinoscope->opencl == NULL) {
        LOG_ERROR_NULL_PTR();
        goto fail_exit;
    }

    /*
     * TODO
     *
     * Configurez les paramètres du noyau OpenCL afin d'envoyer `sinoscope->opencl->buffer` et les
     * autres données nécessaire à son exécution.
     *
     * Démarrez le noyau OpenCL et attendez son exécution.
     *
     * Effectuez la lecture du tampon `sinoscope->opencl->buffer` contenant le résultat dans
     * `sinoscope->buffer`.
     */

    clSetKernelArg(sinoscope->opencl->kernel, 0, sizeof(cl_mem), &(sinoscope->opencl->buffer));



fail_exit:
    return -1;
}
