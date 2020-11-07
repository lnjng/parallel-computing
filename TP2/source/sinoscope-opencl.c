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

    // Initialiser `opencl->context` du périphérique reçu en paramètre.

    opencl->context = clCreateContext(0, 1, &opencl_device_id, NULL, NULL, &error);
    if (error != CL_SUCCESS) {
        LOG_ERROR("failed to initialize OpenCL context");
        return -1;
    }

    // Initialiser `opencl->queue` à partir du contexte précèdent.
    opencl->queue = clCreateCommandQueueWithProperties(opencl->context, opencl_device_id, 0, &error);
    //opencl->queue = clCreateCommandQueue(opencl->context, opencl_device_id, 0, &error);
    if (error != CL_SUCCESS) {
        LOG_ERROR("failed to initialize OpenCL queue");
        return -1;
    }
     
    // Initialiser `opencl->buffer` à partir du context précèdent (width * height * 3).
    const unsigned int buffer_size = 3 * width * height;
    opencl->buffer = clCreateBuffer(opencl->context, CL_MEM_READ_WRITE, buffer_size, NULL, &error);
    
    // Initialiser `opencl->kernel`
    char* buffer = NULL;
    size_t length = 0;
    opencl_load_kernel_code(&buffer, &length);
    cl_program program = clCreateProgramWithSource(opencl->context, 1, (const char**)&buffer, &length, &error);
    free(buffer);
    if (error != CL_SUCCESS) {
        return -1;
    }

    char* options_prefix = "-I ";
    char* opencl_include = __OPENCL_INCLUDE__;
    char* options = malloc(strlen(options_prefix) + strlen(opencl_include) + 1);
    strcpy(options, options_prefix);
    strcat(options, opencl_include);
    error = clBuildProgram(program, 1, &opencl_device_id, options, NULL, NULL);
    if (error != CL_SUCCESS) {
        printf("%d", error);
/*         if (error == CL_BUILD_PROGRAM_FAILURE) {
            // Determine the size of the log
            size_t log_size;
            clGetProgramBuildInfo(program, opencl_device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);

            // Allocate memory for the log
            char *log = (char *) malloc(log_size);

            // Get the log
            clGetProgramBuildInfo(program, opencl_device_id, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);

            // Print the log
            printf("%s\n", log);

            free(log);
        } */
        return -1;
    }
    
    free(options);

    opencl->kernel = clCreateKernel(program, "sinoscope_kernel", &error);
    if (error != CL_SUCCESS) {
        return -1;
    }

    return 0;
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
    clReleaseMemObject(opencl->buffer);
    
}

/*
 * TODO
 *
 * Déclarez les structures partagées avec le noyau OpenCL si nécessaire selon votre énoncé.
 * Utilisez l'attribut `__attribute__((packed))` à vos déclarations.
 */
struct __attribute__((packed)) params_int_t {
    //unsigned int buffer_size;
    //unsigned int width;
    //unsigned int height;
    //unsigned int taylor;
    //unsigned int interval;
    cl_int buffer_size;
    cl_int width;
    cl_int height;
    cl_int taylor;
    cl_int interval;
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
    cl_int error = 0;

    /* Configurez les paramètres du noyau OpenCL afin d'envoyer `sinoscope->opencl->buffer` et les
     * autres données nécessaire à son exécution.
     */
    struct params_int_t params_int;
    params_int.buffer_size = sinoscope->buffer_size;
    params_int.width = sinoscope->width;
    params_int.height = sinoscope->height;
    params_int.taylor = sinoscope->taylor;
    params_int.interval = sinoscope->interval;

    struct params_float_t params_float;
    params_float.interval_inverse = sinoscope->interval_inverse;
    params_float.time = sinoscope->time;
    params_float.max = sinoscope->max;
    params_float.phase0 = sinoscope->phase0;
    params_float.phase1 = sinoscope->phase1;
    params_float.dx = sinoscope->dx;
    params_float.dy = sinoscope->dy;

    error = clSetKernelArg(sinoscope->opencl->kernel, 0, sizeof(cl_mem), &(sinoscope->opencl->buffer));
    error |= clSetKernelArg(sinoscope->opencl->kernel, 1, sizeof(struct params_int_t), &params_int);
    error |= clSetKernelArg(sinoscope->opencl->kernel, 2, sizeof(struct params_float_t), &params_float);
    if (error != CL_SUCCESS) {
        return -1;
    }


     /* Démarrez le noyau OpenCL et attendez son exécution.
     */
    size_t global_size[] = {(size_t)sinoscope->width, (size_t)sinoscope->height};
    // TODO: verify this line below
    error = clEnqueueNDRangeKernel(sinoscope->opencl->queue, sinoscope->opencl->kernel, 2, NULL, global_size, NULL, 0, NULL, NULL);
    if (error != CL_SUCCESS) {
        return -1;
    }
    
    // attente
    error = clFinish(sinoscope->opencl->queue);

     /* Effectuez la lecture du tampon `sinoscope->opencl->buffer` contenant le résultat dans
     * `sinoscope->buffer`.
     */
    error = clEnqueueReadBuffer(sinoscope->opencl->queue, sinoscope->opencl->buffer, CL_TRUE, 0, sinoscope->buffer_size, sinoscope->buffer, 0 , NULL, NULL);
    if (error != CL_SUCCESS) {
        return -1;
    }

    return 0;

fail_exit:
    return -1;
}
