/**
 * APPROXIMATE PATTERN MATCHING
 *
 * main function stub: computes ratios and decides which hybrid approach to call
 *
 */

#include <mpi.h>

// The hybrid approaches implemented:
int patterns_over_ranks_hybrid(int argc, char **argv, int rank, int world_size); // Lino
// ... // Paolo

int main(int argc, char **argv)
{
    int rank, world_size;

    /* MPI Initialization */
    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // TODO: compute ratios and decide which hybrid approach to call

    int res = patterns_over_ranks_hybrid(argc, argv, rank, world_size);

    MPI_Finalize();

    return 0;
}
