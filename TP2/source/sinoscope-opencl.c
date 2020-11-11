#include "log.h"
#include "sinoscope.h"

int sinoscope_opencl_init(sinoscope_opencl_t* opencl, cl_device_id opencl_device_id, unsigned int width,
                          unsigned int height) {

    cl_int error = 0;

    opencl->context = clCreateContext(0, 1, &opencl_device_id, NULL, NULL, &error);
    if (error != CL_SUCCESS) {
        LOG_ERROR("failed to initialize OpenCL context");
        return -1;
    }

    opencl->queue = clCreateCommandQueueWithProperties(opencl->context, opencl_device_id, 0, &error);
    if (error != CL_SUCCESS) {
        LOG_ERROR("failed to initialize OpenCL queue");
        return -1;
    }
     
    const unsigned int buffer_size = 3 * width * height;
    opencl->buffer = clCreateBuffer(opencl->context, CL_MEM_READ_WRITE, buffer_size, NULL, &error);
    
    char* buffer = NULL;
    size_t length = 0;
    opencl_load_kernel_code(&buffer, &length);
    cl_program program = clCreateProgramWithSource(opencl->context, 1, (const char**)&buffer, &length, &error);
    free(buffer);
    if (error != CL_SUCCESS) {
        LOG_ERROR("failed to create OpenCL program");
        return -1;
    }

    char* options_prefix = "-I ";
    char* opencl_include = __OPENCL_INCLUDE__;
    char* options = malloc(strlen(options_prefix) + strlen(opencl_include) + 1);
    strcpy(options, options_prefix);
    strcat(options, opencl_include);
    error = clBuildProgram(program, 1, &opencl_device_id, options, NULL, NULL);
    if (error != CL_SUCCESS) {
        LOG_ERROR("failed to build OpenCL program");
        free(options);
        return -1;
    }
    
    free(options);

    opencl->kernel = clCreateKernel(program, "sinoscope_kernel", &error);
    if (error != CL_SUCCESS) {
        LOG_ERROR("failed to create OpenCL kernel");
        return -1;
    }

    return 0;
}

void sinoscope_opencl_cleanup(sinoscope_opencl_t* opencl) {
    clReleaseKernel(opencl->kernel);
    clReleaseCommandQueue(opencl->queue);
    clReleaseContext(opencl->context);
    clReleaseMemObject(opencl->buffer);
}

struct __attribute__((packed)) params_int_t {
    unsigned int buffer_size;
    unsigned int width;
    unsigned int height;
    unsigned int taylor;
    unsigned int interval;
};

int sinoscope_image_opencl(sinoscope_t* sinoscope) {
    if (sinoscope->opencl == NULL) {
        LOG_ERROR_NULL_PTR();
        goto fail_exit;
    }
    cl_int error = 0;

    struct params_int_t params_int;
    params_int.buffer_size = sinoscope->buffer_size;
    params_int.width = sinoscope->width;
    params_int.height = sinoscope->height;
    params_int.taylor = sinoscope->taylor;
    params_int.interval = sinoscope->interval;

    error = clSetKernelArg(sinoscope->opencl->kernel, 0, sizeof(cl_mem), &(sinoscope->opencl->buffer));
    error |= clSetKernelArg(sinoscope->opencl->kernel, 1, sizeof(struct params_int_t), &params_int);
    error |= clSetKernelArg(sinoscope->opencl->kernel, 2, sizeof(float), &(sinoscope->interval_inverse));
    error |= clSetKernelArg(sinoscope->opencl->kernel, 3, sizeof(float), &(sinoscope->time));
    error |= clSetKernelArg(sinoscope->opencl->kernel, 4, sizeof(float), &(sinoscope->max));
    error |= clSetKernelArg(sinoscope->opencl->kernel, 5, sizeof(float), &(sinoscope->phase0));
    error |= clSetKernelArg(sinoscope->opencl->kernel, 6, sizeof(float), &(sinoscope->phase1));
    error |= clSetKernelArg(sinoscope->opencl->kernel, 7, sizeof(float), &(sinoscope->dx));
    error |= clSetKernelArg(sinoscope->opencl->kernel, 8, sizeof(float), &(sinoscope->dy));

    if (error != CL_SUCCESS) {
        LOG_ERROR("failed to set OpenCL kernel arguments");
        return -1;
    }

    size_t global_size[] = {(size_t)sinoscope->height, (size_t)sinoscope->width};
    error = clEnqueueNDRangeKernel(sinoscope->opencl->queue, sinoscope->opencl->kernel, 2, NULL, global_size, NULL, 0, NULL, NULL);
    if (error != CL_SUCCESS) {
        LOG_ERROR("failed to enqueue and execute OpenCL kernel");
        return -1;
    }
    
    error = clFinish(sinoscope->opencl->queue);
    if (error != CL_SUCCESS) {
        LOG_ERROR("failed to wait for OpenCL kernel");
        return -1;
    }

    error = clEnqueueReadBuffer(sinoscope->opencl->queue, sinoscope->opencl->buffer, CL_TRUE, 0, sinoscope->buffer_size, sinoscope->buffer, 0 , NULL, NULL);
    if (error != CL_SUCCESS) {
        LOG_ERROR("failed to enqueue commands to read buffer");
        return -1;
    }

    return 0;

fail_exit:
    return -1;
}
