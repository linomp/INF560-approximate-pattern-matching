/**
 * APPROXIMATE PATTERN MATCHING
 *
 * INF560
 *
 * MPI Approach #1: distribute the patterns over the MPI ranks
 *
 *
 * Usage:
 * ./apm_mpi_patterns_over_ranks 0 dna/small_chrY.fa $(cat dna/line_chrY.fa)
 *
 */

#include <mpi.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>

#include "utils.h"

char *read_input_file(char *filename, int *size);

int levenshtein(char *s1, char *s2, int len, int *column);

int main(int argc, char **argv)
{
    char **pattern;
    char *filename;
    int approx_factor = 0;
    int nb_patterns = 0;
    int i, j;
    char *buf;
    double t1, t2;
    int n_bytes;
    int *n_matches;

    int mpi_call_result;

    int rank, size;
    int local_matches;

    /* MPI Initialization */
    MPI_Init(&argc, &argv);

    /* Get the rank of the current task and the number
     * of MPI processe
     */
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    /* Parallelism Idea :
     *
     * MPI_Broadcast the buf array (the input database)
     * MPI_Scatter the pattern array (the patterns to search)
     * MPI_Gather in the master process
     * master process reports results
     *
     */

    if (rank == 0)
    {
        // Master process

        /* Check number of arguments */
        if (argc < 4)
        {
            printf("Usage: %s approximation_factor "
                   "dna_database pattern1 pattern2 ...\n",
                   argv[0]);
            return 1;
        }

        /* Get the distance factor */
        approx_factor = atoi(argv[1]);

        /* Grab the filename containing the target text */
        filename = argv[2];

        /* Get the number of patterns that the user wants to search for */
        nb_patterns = argc - 3;

        // make sure there are as many processes as patterns (without counting master)
        if (nb_patterns > (size - 1))
        {
            printf("You need at least %d processes for %d patterns\n", nb_patterns + 1, nb_patterns);
            return 1;
        }

        /* Fill the pattern array */
        pattern = (char **)malloc(nb_patterns * sizeof(char *));
        if (pattern == NULL)
        {
            fprintf(stderr,
                    "Unable to allocate array of pattern of size %d\n",
                    nb_patterns);
            return 1;
        }

        /* Grab the patterns */
        for (i = 0; i < nb_patterns; i++)
        {
            int l;

            l = strlen(argv[i + 3]);
            if (l <= 0)
            {
                fprintf(stderr, "Error while parsing argument %d\n", i + 3);
                return 1;
            }

            pattern[i] = (char *)malloc((l + 1) * sizeof(char));
            if (pattern[i] == NULL)
            {
                fprintf(stderr, "Unable to allocate string of size %d\n", l);
                return 1;
            }

            strncpy(pattern[i], argv[i + 3], (l + 1));
        }

        printf("Approximate Pattern Matching: "
               "looking for %d pattern(s) in file %s w/ distance of %d\n",
               nb_patterns, filename, approx_factor);

        buf = read_input_file(filename, &n_bytes);
        if (buf == NULL)
        {
            return 1;
        }

        /* Allocate the array of matches */
        n_matches = (int *)malloc(nb_patterns * sizeof(int));
        if (n_matches == NULL)
        {
            fprintf(stderr, "Error: unable to allocate memory for %ldB\n",
                    nb_patterns * sizeof(int));
            return 1;
        }

        // Send size of buffer
        mpi_call_result = MPI_Bcast(&n_bytes, 1, MPI_INT, 0, MPI_COMM_WORLD);
        if (!mpi_call_result == MPI_SUCCESS)
        {
            return 1;
        }
        printf("\n(Rank %d) Sent n_bytes=%d\n", rank, n_bytes);

        // send content of buffer
        mpi_call_result = MPI_Bcast(buf, n_bytes, MPI_BYTE, 0, MPI_COMM_WORLD);
        if (!mpi_call_result == MPI_SUCCESS)
        {
            return 1;
        }
        printf("\n(Rank %d) Sent buf=%s\n", rank, buf);

        // TODO: MPI_Scatterv the patterns array

        /* Timer start (from the moment all data is made available to Workers)*/
        t1 = MPI_Wtime();

        MPI_Status status;
        int results_received = 0;
        int temp;
        while (results_received < (size - 1))
        {
            MPI_Recv(&temp, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            results_received++;
            printf("Message from rank %d: %d\n", status.MPI_SOURCE, temp);
            n_matches[status.MPI_SOURCE] = temp;
        }

        /* Timer stop (when results from all Workers are received) */
        t2 = MPI_Wtime();

        printf("(Rank %d) - Total APM Computation time: %f s\n", rank, t2 - t1);

        for (i = 0; i < nb_patterns; i++)
        {
            printf("Number of matches for pattern <%.40s>: %d\n",
                   pattern[i], n_matches[i]);
        }
    }
    else
    {
        // Worker Processes

        // Receive size of buffer
        mpi_call_result = MPI_Bcast(&n_bytes, 1, MPI_INT, 0, MPI_COMM_WORLD);
        if (!mpi_call_result == MPI_SUCCESS)
        {
            return 1;
        }
        printf("\n(Rank %d) Received n_bytes=%d\n", rank, n_bytes);

        // allocate space for buffer
        printf("\n(Rank %d) allocating %d bytes\n", rank, n_bytes * sizeof(char));
        buf = (char *)malloc(n_bytes * sizeof(char));
        if (buf == NULL)
        {
            return 1;
        }

        // get content of buffer
        mpi_call_result = MPI_Bcast(buf, n_bytes, MPI_BYTE, 0, MPI_COMM_WORLD);
        if (!mpi_call_result == MPI_SUCCESS)
        {
            return 1;
        }
        printf("\n(Rank %d) Received buf=%s\n", rank, buf);

        // TODO: receive pattern to look for (Scatterv)

        // TODO: remove this and begin processing (replace looping over all patterns)
        if (0)
        {

            /* Timer start */
            t1 = MPI_Wtime();

            /* Check each pattern one by one */
            for (i = 0; i < nb_patterns; i++)
            {
                int size_pattern = strlen(pattern[i]);
                int *column;

                /* Initialize the number of matches to 0 */
                n_matches[i] = 0;

                column = (int *)malloc((size_pattern + 1) * sizeof(int));
                if (column == NULL)
                {
                    fprintf(stderr, "Error: unable to allocate memory for column (%ldB)\n",
                            (size_pattern + 1) * sizeof(int));
                    return 1;
                }

                /* Traverse the input data up to the end of the file */
                for (j = 0; j < n_bytes; j++)
                {
                    int distance = 0;
                    int size;

#if APM_DEBUG
                    if (j % 100 == 0)
                    {
                        printf("Procesing byte %d (out of %d)\n", j, n_bytes);
                    }
#endif

                    size = size_pattern;
                    if (n_bytes - j < size_pattern)
                    {
                        size = n_bytes - j;
                    }

                    distance = levenshtein(pattern[i], &buf[j], size, column);

                    if (distance <= approx_factor)
                    {
                        n_matches[i]++;
                    }
                }

                free(column);
            }

            /* Timer stop */
            t2 = MPI_Wtime();

            printf("(Rank %d) APM Computation time: %f s\n", rank, t2 - t1);
        }

        // TODO: Send results to master process
        local_matches = rank;
        MPI_Send(&local_matches, 1, MPI_INT, 0, rank, MPI_COMM_WORLD);
    }

    MPI_Finalize();

    return 0;
}
