/**
 * APPROXIMATE PATTERN MATCHING
 *
 * main function stub: computes ratios and decides which hybrid approach to call
 *
 */

#include <math.h>
#include <mpi.h>
#include <omp.h>
#include <stdio.h>
#include <string.h>

#include "approaches.h"

#define DEBUG_APPROACH_CHOSEN 1
#define USE_GPU 1

void getDeviceCount(int *deviceCountPtr);
void setDevice(int rank, int deviceCount);

float getRatio(float x) {
    float ratioHardwareOptimizationApproachChosen;

    if (x < 1) {
        while (x <= 1) {
            x = x * 2;
        }
        ratioHardwareOptimizationApproachChosen = fmod(x, 1.0);
    } else if ((x >= 1) && (x < 2)) {
        ratioHardwareOptimizationApproachChosen = fmod(x, 1.0);
    } else {
        ratioHardwareOptimizationApproachChosen = x;
    }

    return ratioHardwareOptimizationApproachChosen;
}

int main(int argc, char **argv) {
    int rank, world_size;
    int res;
    int mpi_call_result;

    /* MPI Initialization */
    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // In both of our approaches, the master rank only distributes the work to
    // one or more "worker" ranks, and relies on them for actual processing
    if (world_size < 2) {
        if (rank == 0) {
            printf(
                "Minimum number of MPI ranks is 2 (Master Rank is currently "
                "only used to distribute work)\n");
        }
        if (MPI_Finalize() != MPI_SUCCESS) {
            printf("MPI Error: %d\n", mpi_call_result);
            return 1;
        }
        return 1;
    }

    // Check if parallelization approach was explicitly provided (mainly for
    // debugging) or if it must be computed (real usage)
    char *chosen_approach = argv[argc - 1];
    int use_patterns_over_ranks = 0;

    int deviceCount;
    getDeviceCount(&deviceCount);

    // This function call returns 0 if there are no CUDA capable devices.
    setDevice(rank, deviceCount);

    if (!strcmp(chosen_approach, "DB_OVER_RANKS")) {
        // decrease argc so that processing functions ignore last flag
        argc -= 1;
        res = database_over_ranks(argc, argv, rank, world_size);
    } else if (!strcmp(chosen_approach, "PATTERNS_OVER_RANKS")) {
        // decrease argc so that processing functions ignore last flag
        argc -= 1;
        use_patterns_over_ranks = 1;
        res = patterns_over_ranks_hybrid(argc, argv, rank, world_size,
                                         USE_GPU && (deviceCount >= 1));
    } else {
        // Approach not provided, it must be computed
        int n_patterns = argc - 3;
        int omp_threads;
#pragma omp parallel
        { omp_threads = omp_get_num_threads(); }

        int active_ranks = world_size - 1;
        float ratioPatterns = getRatio((float)active_ranks / (float)n_patterns);
        float ratioDatabase = getRatio((float)omp_threads / (float)n_patterns);

#if DEBUG_APPROACH_CHOSEN
        if (rank == 0) {
            printf(
                "jobDimensionPatternsOverRanks = %lf, ratioPatterns = %lf // "
                "jobDimensionDatabaseOverRanks = %lf, ratioDatabase = %lf\n",
                (float)active_ranks / (float)n_patterns,
                (float)omp_threads / (float)n_patterns, ratioPatterns,
                ratioDatabase);
        }
#endif

        // Use Cost Model equations
        if (fabs(ratioPatterns - ratioDatabase) <= 1E-6) {
            // Both of the approaches use the hardware at its
            // maximum capacity, we default to DB-over-ranks approach
            // (random choice creates unnecessary synchronization challenge -
            // all ranks should get the same seed)
            use_patterns_over_ranks = 0;
        } else {
            // We choose the approach that optimizes better the use of
            // hardware.
            if (ratioPatterns < ratioDatabase) {
                use_patterns_over_ranks = 1;
            } else {
                use_patterns_over_ranks = 0;
            }
        }

#if DEBUG_APPROACH_CHOSEN
        if (rank == 0) {
            printf("Approach chosen: %s\n",
                   (use_patterns_over_ranks ? "PATTERNS_OVER_RANKS"
                                            : "DB_OVER_RANKS"));
        }
#endif
        // Call the decided strategy
        if (use_patterns_over_ranks) {
            argc -= 1;
            res = patterns_over_ranks_hybrid(argc, argv, rank, world_size,
                                             USE_GPU && (deviceCount >= 1));
        } else {
            argc -= 1;
            res = database_over_ranks(argc, argv, rank, world_size);
        }
    }

    if (res != 0) {
        printf("%s on Rank %d/%d returned with error %d\n\n",
               (use_patterns_over_ranks ? "PATTERNS_OVER_RANKS"
                                        : "DB_OVER_PATTERNS"),
               rank, world_size, res);
    }

    mpi_call_result = MPI_Finalize();
    if (mpi_call_result != MPI_SUCCESS) {
        printf("MPI Error: %d\n", mpi_call_result);
        return 1;
    }

    return 0;
}
