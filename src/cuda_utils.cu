
// CUDA-C includes
#include <cuda.h>
#include <cuda_runtime.h>

#include <cstdio>

#define DEBUG_CUDA 1

extern "C" void getDeviceCount(int *deviceCountPtr) {
    cudaError_t error_id = cudaGetDeviceCount(deviceCountPtr);

    if (error_id != cudaSuccess) {
        printf("cudaGetDeviceCount returned %d\n-> %s\n",
               static_cast<int>(error_id), cudaGetErrorString(error_id));
        printf("Result = FAIL\n");
        exit(EXIT_FAILURE);
    }
    return;
}

extern "C" void setDevice(int rank, int deviceCount) {
    if (deviceCount == 0) {
        printf("There are no available device(s) that support CUDA\n");
    } else {
#ifdef DEBUG_CUDA
        printf(
            "Rank %d detected %d CUDA Capable device(s) - performing "
            "cudaSetDevice(0)\n",
            rank, deviceCount);
#endif
        cudaSetDevice(0);
    }
    return;
}