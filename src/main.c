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

    // TODO: compute ratios and decide which hybrid approach to call
    int res;

#ifdef PAOLO
    res = database_over_ranks(argc, argv, rank, world_size);
#else
    res = patterns_over_ranks_hybrid(argc, argv, rank, world_size);
#endif

    // Validate functions return 0, or check for error

    MPI_Finalize();

    return 0;
}

// TODO: validate world_size > 1 (in both of our approaches master rank does not perform work)
// TODO: some MPI nodes/process combinations throw errors??
