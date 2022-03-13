/**
 * APPROXIMATE PATTERN MATCHING
 *
 * main function stub: computes ratios and decides which hybrid approach to call
 *
 */

#include <mpi.h>

#include "approaches.h"

int main(int argc, char **argv) {
    int rank, world_size;

    /* MPI Initialization */
    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int res;
    int mpi_call_result;
    char *chosen_approach = argv[argc - 1];

    if (!strcmp(chosen_approach, "DB_OVER_RANKS")) {
        // argc-- so that processing functions ignore last flag
        res = database_over_ranks(argc--, argv, rank, world_size);
    } else if (!strcmp(chosen_approach, "PATTERNS_OVER_RANKS")) {
        // argc-- so that processing functions ignore last flag
        res = patterns_over_ranks_hybrid(argc--, argv, rank, world_size);
    } else {
        // Approach not provided, it must be computed

        int omp_threads;
        int n_patterns = argc - 3;
        int use_patterns_over_ranks = 0;

#pragma omp parallel
        { omp_threads = omp_get_num_threads(); }
        printf("OMP THREADS: %d\n", omp_threads);

#ifdef USE_GPU
        // Use MPI + OpenMP + GPU equations
#else
        // Use MPI + OpenMP equations

        // 1 Pattern
        if (n_patterns == 1) {
            int active_ranks = world_size - 1;
            int lino_lost_threads = (active_ranks - 1) * omp_threads;
            int paolo_lost_threads = active_ranks * (omp_threads - 1);
            use_patterns_over_ranks =
                (lino_lost_threads < paolo_lost_threads) ? 1 : 0;
        }

        // Multiple Patterns

        // TODO: compute ratios and decide which hybrid approach to call

#ifdef DEBUG_APPROACH_CHOSEN
        printf("Approach chosen: %s\n",
               (use_patterns_over_ranks ? "PATTERNS_OVER_RANKS"
                                        : "DB_OVER_RANKS"));
#endif
        // Call the decided strategy
        if (use_patterns_over_ranks) {
            res = patterns_over_ranks_hybrid(argc--, argv, rank, world_size);
        } else {
            res = database_over_ranks(argc--, argv, rank, world_size);
        }
#endif
    }

#ifdef APM_DEBUG
    printf("%s on Rank %d/%d returned %d\n\n", chosen_approach, rank,
           world_size, res);
#endif

    mpi_call_result = MPI_Finalize();
    if (mpi_call_result != MPI_SUCCESS) {
        printf("MPI Error: %d\n", mpi_call_result);
        return 1;
    }

    return 0;
}

// TODO: validate world_size > 1 (in both of our approaches master rank does not
// perform work)
// TODO Validate processing functions return 0, or check for error
