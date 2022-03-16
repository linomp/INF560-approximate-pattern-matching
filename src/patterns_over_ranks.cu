
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

// CUDA-C includes
#include <cuda.h>
#include <cuda_runtime.h>

#include <cstdio>

#define DEBUG_CUDA 1

#define MIN3(a, b, c) \
    ((a) < (b) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c)))

__global__ void ComputeMatches(char *buf, char *pattern, int *local_matches,
                               int n_bytes, int pattern_length,
                               int approx_factor) {
    // Source of tip about using pragma unroll:
    // https://docs.nvidia.com/cuda/cuda-c-best-practices-guide/index.html#branch-predication

    // For loop inside kernel source:
    // https://www.diehlpk.de/blog/cuda-7-forall/

    int distance = 0;
    int j;

    int *column = (int *)malloc((pattern_length + 1) * sizeof(int));

    for (j = blockDim.x * blockIdx.x + threadIdx.x; j < n_bytes - approx_factor;
         j += gridDim.x * blockDim.x) {
        int size;

        size = pattern_length;
        if (n_bytes - j < pattern_length) {
            size = n_bytes - j;
        }

        // Levenshtein
        unsigned int x, y, lastdiag, olddiag;

#pragma unroll
        for (y = 1; y <= size; y++) {
            column[y] = y;
        }
#pragma unroll
        for (x = 1; x <= size; x++) {
            column[0] = x;
            lastdiag = x - 1;
            for (y = 1; y <= size; y++) {
                olddiag = column[y];
                column[y] = MIN3(
                    column[y] + 1, column[y - 1] + 1,
                    lastdiag + (pattern[y - 1] == (&buf[j])[x - 1] ? 0 : 1));
                lastdiag = olddiag;
            }
        }

        distance = column[size];

        if (distance <= approx_factor) {
            (*local_matches)++;
        }
    }

    free(column);
}

extern "C" int *invoke_kernel(char *buf, int n_bytes, char *my_pattern,
                              int pattern_length, int approx_factor,
                              int *local_matches) {
    // Allocate arrays in device memory
    char *d_buf;
    cudaMalloc(&d_buf, n_bytes);
    char *d_pattern;
    cudaMalloc(&d_pattern, pattern_length);
    int *d_local_matches;
    cudaMalloc(&d_local_matches, 1 * sizeof(int));

#if DEBUG_CUDA
    printf("DEBUG_CUDA: Starting memory transfers...\n");
#endif

    // Copy buffer & pattern from host memory to device memory
    cudaMemcpy(d_buf, buf, n_bytes, cudaMemcpyHostToDevice);
    cudaMemcpy(d_pattern, my_pattern, pattern_length, cudaMemcpyHostToDevice);
    cudaMemcpy(d_local_matches, local_matches, 1 * sizeof(int),
               cudaMemcpyHostToDevice);

    int threadsPerBlock = 256;
    int blocksPerGrid = (n_bytes + threadsPerBlock - 1) / threadsPerBlock;

    ComputeMatches<<<blocksPerGrid, threadsPerBlock>>>(
        d_buf, d_pattern, d_local_matches, n_bytes, pattern_length,
        approx_factor);

#if DEBUG_CUDA
    printf("DEBUG_CUDA: Kernel invoked - &d_local_matches=%ld\n",
           d_local_matches);
#endif

    // Free device memory
    cudaFree(d_buf);
    cudaFree(d_pattern);

    return d_local_matches;
}

extern "C" void write_kernel_result(int *local_matches, int *d_local_matches) {
#if DEBUG_CUDA
    printf(
        "DEBUG_CUDA: getting result from device address &d_local_matches=%ld\n",
        d_local_matches);
#endif

    // Copy result from device memory to host memory
    cudaMemcpy(local_matches, d_local_matches, 1 * sizeof(int),
               cudaMemcpyDeviceToHost);

#if DEBUG_CUDA
    printf("DEBUG_CUDA: Matches found = %d\n", *local_matches);
#endif

    // Free remaining device pointer
    cudaFree(d_local_matches);

    return;
}
