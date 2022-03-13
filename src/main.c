/**
 * APPROXIMATE PATTERN MATCHING
 *
 * main function stub: computes ratios and decides which hybrid approach to call
 *
 */

#include <mpi.h>

#include "approaches.h"

int main(int argc, char **argv)
{
    int rank, world_size;

    /* MPI Initialization */
    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int res;
    int mpi_call_result;
    char *chosen_approach = argv[argc - 1];

    // So that processing functions ignore last flag
    argc--;

    // TODO: compute ratios and decide which hybrid approach to call

    if (!strcmp(chosen_approach, "DB_OVER_RANKS"))
    {
        res = database_over_ranks(argc, argv, rank, world_size);
    }
    else if (!strcmp(chosen_approach, "PATTERNS_OVER_RANKS"))
    {
        res = patterns_over_ranks_hybrid(argc, argv, rank, world_size);
    }

#ifdef APM_DEBUG
    printf("%s on Rank %d/%d returned %d\n\n", chosen_approach, rank, world_size, res);
#endif

    mpi_call_result = MPI_Finalize();
    if (mpi_call_result != MPI_SUCCESS)
    {
        printf("MPI Error: %d\n", mpi_call_result);
        return 1;
    }

    return 0;
}

// TODO: validate world_size > 1 (in both of our approaches master rank does not perform work)
// TODO Validate processing functions return 0, or check for error
