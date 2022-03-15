
// CUDA-C includes
#include <cuda.h>
#include <cuda_runtime.h>

#include <cstdio>

#define DEBUG_CUDA 1

extern "C" int search_pattern_kernel(int n_bytes) { return n_bytes; }