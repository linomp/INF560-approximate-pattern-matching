/**
 * APPROXIMATE PATTERN MATCHING
 *
 * INF560
 *
 * Hybrid Approach #1: distribute the patterns over the MPI ranks;
 *                     parallelize the processing of a pattern within a rank
 *
 * To turn on processing-time reporting, add -DAPM_INFO to the CFLAGS property in Makefile
 * Otherwise, it just outputs the pattern matching results as expected by the automated test scripts
 *
 */

#include <mpi.h>
#include <omp.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>

#include "utils.h"

void broadcast_check_result(int *value)
{
    /* The Master Rank will use this function to broadcast all others that something wrong,
     * so they can return as well and the main function will call MPI_Finalize().
     *
     * This is needed because we decided to move the processing strategies to separate functions.
     */
    MPI_Bcast(value, 1, MPI_INT, 0, MPI_COMM_WORLD);
}

int patterns_over_ranks_hybrid(int argc, char **argv, int rank, int world_size)
{

    char **pattern;
    char *filename;
    int approx_factor = 0;
    int nb_patterns = 0;
    int i;
    char *buf;
    double t1, t2;
    int n_bytes;
    int *n_matches;

    int mpi_call_result;
    MPI_Status status;
    int local_matches;

    int checks_ok = 0;

#ifdef APM_DEBUG
    printf("World size: %d | My rank: %d\n", world_size, rank);
#endif

    /* Check number of arguments */
    if (argc < 4)
    {
        printf("Usage: %s approximation_factor "
               "dna_database pattern1 pattern2 ...\n",
               argv[0]);

        checks_ok = 0;
        // broadcast_check_result(&checks_ok);
        return 1;
    }

    /* Get the distance factor */
    approx_factor = atoi(argv[1]);

    /* Grab the filename containing the target text */
    filename = argv[2];

    /* Get the number of patterns that the user wants to search for */
    nb_patterns = argc - 3;

    // make sure there are as many processes as patterns (without counting master)
    if (nb_patterns != (world_size - 1))
    {
        printf("You need %d processes for %d patterns\n", nb_patterns + 1, nb_patterns);

        checks_ok = 0;
        broadcast_check_result(&checks_ok);
        return 1;
    }

    if (rank == 0)
    {
        // Master process

        /* Fill the pattern array */
        pattern = (char **)malloc(nb_patterns * sizeof(char *));
        if (pattern == NULL)
        {
            fprintf(stderr,
                    "Unable to allocate array of pattern of size %d\n",
                    nb_patterns);

            checks_ok = 0;
            broadcast_check_result(&checks_ok);
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

                checks_ok = 0;
                broadcast_check_result(&checks_ok);
                return 1;
            }

            pattern[i] = (char *)malloc((l + 1) * sizeof(char));
            if (pattern[i] == NULL)
            {
                fprintf(stderr, "Unable to allocate string of size %d\n", l);

                checks_ok = 0;
                broadcast_check_result(&checks_ok);
                return 1;
            }

            strncpy(pattern[i], argv[i + 3], (l + 1));
        }

#ifdef APM_INFO
        printf("\n\nApproximate Pattern Matching: "
               "looking for %d pattern(s) in file %s w/ distance of %d\n\n",
               nb_patterns, filename, approx_factor);
#endif

        buf = read_input_file(filename, &n_bytes);
        if (buf == NULL)
        {
            checks_ok = 0;
            broadcast_check_result(&checks_ok);
            return 1;
        }

        /* Allocate the array of matches */
        n_matches = (int *)malloc(nb_patterns * sizeof(int));
        if (n_matches == NULL)
        {
            fprintf(stderr, "Error: unable to allocate memory for %ldB\n",
                    nb_patterns * sizeof(int));

            checks_ok = 0;
            broadcast_check_result(&checks_ok);
            return 1;
        }

        // Broadcast that checks went OK
        checks_ok = 1;
        broadcast_check_result(&checks_ok);
#ifdef APM_DEBUG
        printf("Rank %d - sent Checks OK\n", rank);
#endif

#ifdef APM_INFO
        /* Timer start (from the moment data distribution begins)*/
        t1 = MPI_Wtime();
#endif

        // Send size of buffer
        n_bytes++;
        buf[n_bytes - 1] = '\0';
        mpi_call_result = MPI_Bcast(&n_bytes, 1, MPI_INT, 0, MPI_COMM_WORLD);
        if (mpi_call_result != MPI_SUCCESS)
        {
            return 1;
        }
#ifdef APM_DEBUG
        printf("\n(Rank %d) Sent n_bytes=%d\n", rank, n_bytes);
#endif

        // send content of buffer
        mpi_call_result = MPI_Bcast(buf, n_bytes, MPI_BYTE, 0, MPI_COMM_WORLD);
        if (mpi_call_result != MPI_SUCCESS)
        {
            return 1;
        }
#ifdef APM_DEBUG
        printf("\n(Rank %d) Sent buf=%s\n", rank, buf);
#endif

        MPI_Barrier(MPI_COMM_WORLD);

        // Distribute the patterns
        for (i = 0; i < nb_patterns; i++)
        {
            int dest_rank = i + 1;                       // skip master process
            int pattern_length = strlen(pattern[i]) + 1; // account for '\0'
            pattern[pattern_length - 1] = '\0';

            mpi_call_result = MPI_Ssend(&pattern_length, 1, MPI_INT, dest_rank, 0, MPI_COMM_WORLD);
            if (mpi_call_result != MPI_SUCCESS)
            {
                return 1;
            }

            mpi_call_result = MPI_Ssend(pattern[i], pattern_length, MPI_BYTE, dest_rank, 0, MPI_COMM_WORLD);
            if (mpi_call_result != MPI_SUCCESS)
            {
                return 1;
            }
        }

        int results_received = 0;
        int temp;
        while (results_received < (world_size - 1))
        {
            MPI_Recv(&temp, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            results_received++;
#ifdef APM_DEBUG
            printf("Message from rank %d: %d\n", status.MPI_SOURCE, temp);
#endif
            int processed_pattern_idx = status.MPI_SOURCE - 1;
            n_matches[processed_pattern_idx] = temp;
        }

#ifdef APM_INFO
        /* Timer stop (when results from all Workers are received) */
        t2 = MPI_Wtime();
        printf("(Rank %d) - Total time using %d mpi_ranks and %d omp_thread(s) per rank: %f s\n\n", rank, world_size, atoi(getenv("OMP_NUM_THREADS")), t2 - t1);
#endif
        for (i = 0; i < nb_patterns; i++)
        {
            printf("Number of matches for pattern <%.40s>: %d\n",
                   pattern[i], n_matches[i]);
        }
    }
    else
    {
        // Worker Processes

        // Receive confirmation that checks doen by master rank succeeded
#ifdef APM_DEBUG
        printf("Rank %d - waiting for checks result broadcast\n", rank);
#endif
        mpi_call_result = MPI_Bcast(&checks_ok, 1, MPI_INT, 0, MPI_COMM_WORLD);
        if ((!checks_ok) || (mpi_call_result != MPI_SUCCESS))
        {
#ifdef APM_DEBUG
            printf("Rank %d - received Checks FAILED\n", rank);
#endif
            return 1;
        }
#ifdef APM_DEBUG
        printf("Rank %d - received Checks OK\n", rank);
#endif

        // Receive size of buffer
        mpi_call_result = MPI_Bcast(&n_bytes, 1, MPI_INT, 0, MPI_COMM_WORLD);
        if (mpi_call_result != MPI_SUCCESS)
        {
            return 1;
        }
#ifdef APM_DEBUG
        printf("\n(Rank %d) Received n_bytes=%d\n", rank, n_bytes);
#endif

// allocate space for buffer
#ifdef APM_DEBUG
        printf("\n(Rank %d) allocating %d bytes\n", rank, n_bytes * sizeof(char));
#endif
        buf = (char *)malloc(n_bytes * sizeof(char));
        if (buf == NULL)
        {
            return 1;
        }

        // get content of buffer
        mpi_call_result = MPI_Bcast(buf, n_bytes, MPI_BYTE, 0, MPI_COMM_WORLD);
        if (mpi_call_result != MPI_SUCCESS)
        {
            return 1;
        }
        buf[n_bytes] = '\0';
#ifdef APM_DEBUG
        printf("\n(Rank %d) Received buf=%s\n", rank, buf);
#endif

        MPI_Barrier(MPI_COMM_WORLD);

        // Receive size of pattern
        int pattern_length;
        mpi_call_result = MPI_Recv(&pattern_length, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        if (mpi_call_result != MPI_SUCCESS)
        {
            return 1;
        }
#ifdef APM_DEBUG
        printf("\n(Rank %d) Received pattern_length=%d\n", rank, pattern_length);
#endif

        // Allocate space for pattern
        char *my_pattern = (char *)malloc((pattern_length * 2) * sizeof(char));
        if (my_pattern == NULL)
        {
            return 1;
        }

        // Receive content of pattern
        mpi_call_result = MPI_Recv(my_pattern, pattern_length, MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        if (mpi_call_result != MPI_SUCCESS)
        {
            return 1;
        }

#ifdef APM_DEBUG
        printf("\n(Rank %d) Received pattern=%s\n", rank, my_pattern);
#endif

        // Begin Processing assigned pattern

        /* Initialize the number of matches to 0 */
        local_matches = 0;
        int pattern_size = pattern_length - 1; // discount the null-terminator

        /* Process the input data with team of OpenMP Threads */
#pragma omp parallel default(none) firstprivate(n_bytes, approx_factor, pattern_size, my_pattern) shared(buf, local_matches)
        {
            n_bytes = n_bytes;
            approx_factor = approx_factor;
            pattern_size = pattern_size;
            my_pattern = my_pattern;

            int j;

            int chunk_size = (2 * pattern_size) - 1; // offset for ghost cells

            int *column = (int *)malloc((chunk_size + 1) * sizeof(int));

#ifdef APM_DEBUG
            printf("thread: %d - chunk_size: %d\n", omp_get_thread_num(), chunk_size);
#endif

#pragma omp for schedule(dynamic, chunk_size)
            for (j = 0; j < n_bytes - approx_factor; j++)
            {
                int distance = 0;
                int size;

                size = pattern_size;
                if (n_bytes - j < pattern_size)
                {
                    size = n_bytes - j;
                }

                distance = levenshtein(my_pattern, &buf[j], size, column);

                if (distance <= approx_factor)
                {
                    local_matches++;
                }
            }
            free(column);
        }

        MPI_Send(&local_matches, 1, MPI_INT, 0, rank, MPI_COMM_WORLD);
    }

    return 0;
}

// TODO: add some sort of round robin or load balancing to support more patterns than processes & viceversa
// TODO: fix bug - patterns of different lengths w. dummy db!