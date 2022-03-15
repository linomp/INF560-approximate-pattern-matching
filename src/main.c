/**
 * APPROXIMATE PATTERN MATCHING
 *
 * main function stub: computes ratios and decides which hybrid approach to call
 *
 */

#include <math.h>
#include <mpi.h>
#include <stdio.h>

#include "approaches.h"

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
    if (world_size <= 2) {
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

    if (!strcmp(chosen_approach, "DB_OVER_RANKS")) {
        // decrease argc so that processing functions ignore last flag
        argc -= 1;
        res = database_over_ranks(argc, argv, rank, world_size);
    } else if (!strcmp(chosen_approach, "PATTERNS_OVER_RANKS")) {
        // decrease argc so that processing functions ignore last flag
        argc -= 1;
        use_patterns_over_ranks = 1;
        res = patterns_over_ranks_hybrid(argc, argv, rank, world_size);
    } else {
        // Approach not provided, it must be computed
        int n_patterns = argc - 3;
        int omp_threads;
#pragma omp parallel
        { omp_threads = omp_get_num_threads(); }

        int active_ranks = world_size - 1;
        float ratioPatterns = getRatio((float)active_ranks / (float)n_patterns);
        float ratioDatabase = getRatio((float)omp_threads / (float)n_patterns);

#ifdef DEBUG_APPROACH_CHOSEN
        if (rank == 0) {
            printf("ratioPatterns = %lf // ratioDatabase = %lf\n",
                   ratioPatterns, ratioDatabase);
        }
#endif

#ifdef USE_GPU
        // Use MPI + OpenMP + GPU equations
#else
        // Use MPI + OpenMP equations

        if (ratioPatterns == 0 && ratioDatabase == 0) {
            // It's the same. Both of the approaces use the hardware at its
            // maximum capacity.
            // TODO: random(DatabaseOverRanks, PatternsOverRanks);
            use_patterns_over_ranks = 1;
        } else {
            // We choose the approach that optimizes better the use of
            // hardware.
            if (ratioPatterns < ratioDatabase) {
                use_patterns_over_ranks = 1;
            } else {
                use_patterns_over_ranks = 0;
            }
        }

#ifdef DEBUG_APPROACH_CHOSEN
        if (rank == 0) {
            printf("Approach chosen: %s\n",
                   (use_patterns_over_ranks ? "PATTERNS_OVER_RANKS"
                                            : "DB_OVER_RANKS"));
        }
#endif
        // Call the decided strategy
        if (use_patterns_over_ranks) {
            argc -= 1;
            res = patterns_over_ranks_hybrid(argc, argv, rank, world_size);
        } else {
            argc -= 1;
            res = database_over_ranks(argc, argv, rank, world_size);
        }
#endif
    }

#ifdef APM_DEBUG
    printf(
        "%s on Rank %d/%d returned %d\n\n",
        (use_patterns_over_ranks ? "PATTERNS_OVER_RANKS" : "DB_OVER_PATTERNS"),
        rank, world_size, res);
#endif

    mpi_call_result = MPI_Finalize();
    if (mpi_call_result != MPI_SUCCESS) {
        printf("MPI Error: %d\n", mpi_call_result);
        return 1;
    }

    return 0;
}
