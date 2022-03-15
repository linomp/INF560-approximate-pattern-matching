
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

// CUDA-C includes
#include <cuda.h>
#include <cuda_runtime.h>

#include <cstdio>

#define DEBUG_CUDA 1
#define TESTPERFORMANCE_NO_LEVENSHTEIN 1

#define MIN3(a, b, c) \
    ((a) < (b) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c)))

/* Function computing the final string to print */
__global__ void ComputeMatches(char *buf, char *pattern, int *local_matches,
                               int n_bytes, int pattern_length,
                               int approx_factor) {
    int distance = 0;

    int j = blockDim.x * blockIdx.x + threadIdx.x;

#if TESTPERFORMANCE_NO_LEVENSHTEIN
    // unsigned int ns = 8;
    //__nanosleep(ns);
    return;
#else
    int *column = (int *)malloc((pattern_length + 1) * sizeof(int));

    for (j = blockDim.x * blockIdx.x + threadIdx.x;
         j < (n_bytes / blockDim.x) - approx_factor; j++) {
        int size;

        size = pattern_length;
        if (n_bytes - j < pattern_length) {
            size = n_bytes - j;
        }

        // Levenshtein
        unsigned int x, y, lastdiag, olddiag;

        for (y = 1; y <= size; y++) {
            column[y] = y;
        }
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
#endif
}

extern "C" int search_pattern_kernel(char *buf, int n_bytes, char *my_pattern,
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

    // Copy result from device memory to host memory
    cudaMemcpy(local_matches, d_local_matches, 1 * sizeof(int),
               cudaMemcpyDeviceToHost);

#if DEBUG_CUDA
    printf("DEBUG_CUDA: Matches found in the first %d bytes = %d\n", n_bytes,
           *local_matches);
#endif

    // Free device memory
    cudaFree(d_buf);
    cudaFree(d_pattern);
    cudaFree(local_matches);

    return 0;
}

/*
#pragma omp parallel default(none)                                         \
    firstprivate(rank, n_bytes, approx_factor, pattern_length, my_pattern, \
                 cuda_device_exists) shared(buf, local_matches)
{
    rank = rank;

    // Overall idea: if there is a cuda device, omp threads take on
    // just half of the workload + "ghost cells"
    n_bytes =
        cuda_device_exists ? ((n_bytes / 2) + (pattern_length - 1)) :
n_bytes;

    approx_factor = approx_factor;
    pattern_length = pattern_length;
    my_pattern = my_pattern;

    int j;

    int chunk_size = (2 * pattern_length) - 1;  // offset for ghost cells

    int *column = (int *)malloc((chunk_size + 1) * sizeof(int));

#if APM_DEBUG
    printf("thread: %d - chunk_size: %d\n", omp_get_thread_num(),
chunk_size); #endif printf("Starting with local matches = %d\n",
local_matches);

#pragma omp for schedule(dynamic, chunk_size)
    for (j = 0; j < n_bytes - approx_factor; j++) {
#if APM_DEBUG_BYTES
        printf("(Rank %d - Thread %d) - processing byte %d\n", rank,
               omp_get_thread_num(), j);
#endif
        int distance = 0;
        int size;

        size = pattern_length;
        if (n_bytes - j < pattern_length) {
            size = n_bytes - j;
        }

        distance = levenshtein(my_pattern, &buf[j], size, column);

        if (distance <= approx_factor) {
            local_matches++;
        }
    }
    free(column);
}*/