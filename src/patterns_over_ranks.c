/**
 * APPROXIMATE PATTERN MATCHING
 *
 * INF560
 *
 * Hybrid Approach #1: distribute the patterns over the MPI ranks;
 *                     parallelize the processing of a pattern within a rank
 *
 * To turn on processing-time reporting, add -DAPM_INFO to the CFLAGS property
 * in Makefile Otherwise, it just outputs the pattern matching results as
 * expected by the automated test scripts
 *
 */

#include <fcntl.h>
#include <mpi.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "approaches.h"
#include "utils.h"

#define APM_INFO 1
#define APM_DEBUG 0
#define APM_DEBUG_BUF 0
#define APM_DEBUG_ALLOC 0
#define APM_DEBUG_BYTES 0

int *invoke_kernel(char *buf, int n_bytes, char *my_pattern, int pattern_length,
                   int approx_factor, int *local_matches);

void write_kernel_result(int *local_matches, int *d_local_matches);

int patterns_over_ranks_hybrid(int argc, char **argv, int rank, int world_size,
                               int cuda_device_exists) {
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

    int tag;

#if APM_DEBUG
    printf("World size: %d | My rank: %d\n", world_size, rank);
#endif

    /* Check number of arguments */
    if (argc < 4) {
        printf(
            "Usage: %s approximation_factor "
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

    if (rank == 0) {
        // Master process

        /* Fill the pattern array */
        pattern = (char **)malloc(nb_patterns * sizeof(char *));
        if (pattern == NULL) {
            fprintf(stderr, "Unable to allocate array of pattern of size %d\n",
                    nb_patterns);

            return 1;
        }

        /* Grab the patterns */
        for (i = 0; i < nb_patterns; i++) {
            int l;

            l = strlen(argv[i + 3]);
            if (l <= 0) {
                fprintf(stderr, "Error while parsing argument %d\n", i + 3);

                return 1;
            }

            pattern[i] = (char *)malloc((l + 1) * sizeof(char));
            if (pattern[i] == NULL) {
                fprintf(stderr, "Unable to allocate string of size %d\n", l);

                return 1;
            }

            strncpy(pattern[i], argv[i + 3], (l + 1));
        }

#if APM_INFO
        printf(
            "Approximate Pattern Matching: "
            "looking for %d pattern(s) in file %s w/ distance of %d\n\n",
            nb_patterns, filename, approx_factor);
#endif

        buf = read_input_file(filename, &n_bytes);
        if (buf == NULL) {
            return 1;
        }

        /* Allocate the array of matches */
        n_matches = (int *)malloc(nb_patterns * sizeof(int));
        if (n_matches == NULL) {
            fprintf(stderr, "Error: unable to allocate memory for %ldB\n",
                    nb_patterns * sizeof(int));

            return 1;
        }

#if APM_INFO
        /* Timer start (from the moment data distribution begins)*/
        t1 = MPI_Wtime();
#endif

        // Send size of buffer
        mpi_call_result = MPI_Bcast(&n_bytes, 1, MPI_INT, 0, MPI_COMM_WORLD);
        if (mpi_call_result != MPI_SUCCESS) {
            printf("MPI Error: %d\n", mpi_call_result);
            return 1;
        }
#if APM_DEBUG
        printf("\n(Rank %d) Sent n_bytes=%d\n", rank, n_bytes);
#endif

        // send content of buffer
        mpi_call_result = MPI_Bcast(buf, n_bytes, MPI_BYTE, 0, MPI_COMM_WORLD);
        if (mpi_call_result != MPI_SUCCESS) {
            printf("MPI Error: %d\n", mpi_call_result);
            return 1;
        }
#if APM_DEBUG_BUF
        printf("\n(Rank %d) Sent buf=%s\n", rank, "buffer");
#endif

        // Distribute the patterns accross available ranks (round-robin
        // scheduling)
        for (i = 0; i < nb_patterns; i++) {
            int dest_rank = 1 + (i % (world_size - 1));  // skip master process
            int pattern_length = strlen(pattern[i]);

            tag = i;

#if APM_DEBUG
            printf("Master sending pattern %d to rank %d\n", tag, dest_rank);
#endif
            mpi_call_result = MPI_Send(&pattern_length, 1, MPI_INT, dest_rank,
                                       0, MPI_COMM_WORLD);
            if (mpi_call_result != MPI_SUCCESS) {
                printf("MPI Error: %d\n", mpi_call_result);
                return 1;
            }

            mpi_call_result = MPI_Send(pattern[i], pattern_length, MPI_BYTE,
                                       dest_rank, tag, MPI_COMM_WORLD);
            if (mpi_call_result != MPI_SUCCESS) {
                printf("MPI Error: %d\n", mpi_call_result);
                return 1;
            }
        }

        int dest_rank;

        /* recv the results */
        int temp;
        for (i = 0; i < nb_patterns; i++) {
            dest_rank = 1 + (i % (world_size - 1));
#if APM_DEBUG
            printf("Master waiting for result from rank%d: n_matches[%d] = ?\n",
                   dest_rank, i);
#endif

            mpi_call_result = MPI_Recv(&temp, 1, MPI_INT, MPI_ANY_SOURCE,
                                       MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            if (mpi_call_result != MPI_SUCCESS) {
                printf("MPI Error: %d\n", mpi_call_result);
                return 1;
            }
#if APM_DEBUG
            printf("Message from rank %d: n_matches[%d] = %d\n",
                   status.MPI_SOURCE, status.MPI_TAG, temp);
#endif
            int processed_pattern_idx = status.MPI_TAG;
            n_matches[processed_pattern_idx] = temp;
        }

        /* send a negative pattern size to tell the workers to stop */
        int stop_value = -1;
        for (dest_rank = 1; dest_rank < world_size; dest_rank++) {
            mpi_call_result =
                MPI_Send(&stop_value, 1, MPI_INT, dest_rank, 0, MPI_COMM_WORLD);
            if (mpi_call_result != MPI_SUCCESS) {
                printf("MPI Error: %d\n", mpi_call_result);
                return 1;
            }
        }

#if APM_INFO
        /* Timer stop (when results from all Workers are received) */
        t2 = MPI_Wtime();
        printf(
            "\n(Rank %d) - TOTAL TIME using %d mpi_ranks and %d omp_thread(s) "
            "per rank: %f s\n\n",
            rank, world_size, atoi(getenv("OMP_NUM_THREADS")), t2 - t1);
#endif
        for (i = 0; i < nb_patterns; i++) {
            printf("Number of matches for pattern <%.100s>: %d\n", pattern[i],
                   n_matches[i]);
        }
    } else {
        // Worker Processes

        // Initialization: get buffer & buffer content

        // Receive size of buffer
        mpi_call_result = MPI_Bcast(&n_bytes, 1, MPI_INT, 0, MPI_COMM_WORLD);
        if (mpi_call_result != MPI_SUCCESS) {
            printf("MPI Error: %d\n", mpi_call_result);
            return 1;
        }

        // allocate space for buffer
#if APM_DEBUG_ALLOC
        printf("\n(Rank %d) allocating %d bytes\n", rank,
               n_bytes * sizeof(char));
#endif
        buf = (char *)malloc(n_bytes * sizeof(char));
        if (buf == NULL) {
            return 1;
        }

        // get content of buffer & set NUL terminator
        mpi_call_result = MPI_Bcast(buf, n_bytes, MPI_BYTE, 0, MPI_COMM_WORLD);
        if (mpi_call_result != MPI_SUCCESS) {
            printf("MPI Error: %d\n", mpi_call_result);
            return 1;
        }

#if APM_DEBUG_BUF
        printf("\n(Rank %d) Received buf=%s\n", rank, "buffer");
#endif

        // Standby: wait to receive a pattern to search
        while (1) {
            // Receive size of pattern
            // negative value received indicates worker to stop
            int pattern_length;
            mpi_call_result = MPI_Recv(&pattern_length, 1, MPI_INT, 0,
                                       MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            if (mpi_call_result != MPI_SUCCESS) {
                printf("MPI Error: %d\n", mpi_call_result);
                return 1;
            }
#if APM_DEBUG
            printf("\n(Rank %d) Received pattern_length=%d\n", rank,
                   pattern_length);
#endif
            if (pattern_length < 0) {
                /* no more task */
                break;
            }

            // Allocate space for pattern
            char *my_pattern =
                (char *)malloc((pattern_length * 2) * sizeof(char));
            if (my_pattern == NULL) {
                return 1;
            }

            // Receive content of pattern
            mpi_call_result =
                MPI_Recv(my_pattern, pattern_length, MPI_BYTE, MPI_ANY_SOURCE,
                         MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            if (mpi_call_result != MPI_SUCCESS) {
                printf("MPI Error: %d\n", mpi_call_result);
                return 1;
            }
            tag = status.MPI_TAG;

#if APM_DEBUG
            printf("\n(Rank %d) Received pattern=%s with tag: %d\n", rank,
                   my_pattern, tag);
#endif

            // Begin Processing assigned pattern

            /* Initialize the number of matches to 0 */
            local_matches = 0;
            int *device_result_address;
            int device_result = 0;

            // Best ratio determined by experiments is 75%
            //#ifdef GPU_JOB_SIZE_75
            int gpu_job_size = (3 * n_bytes / 4);
            //#else
            //            int gpu_job_size = n_bytes / 2;
            //#endif

            // Overall idea: if there is a cuda device, it takes on
            // the first half of the workload + "ghost cells"
            if (cuda_device_exists) {
                device_result_address = invoke_kernel(
                    buf, gpu_job_size + (pattern_length - 1), my_pattern,
                    pattern_length, approx_factor, &device_result);
            }

            /* Process the input data with OpenMP Threads */
#pragma omp parallel default(none)                                         \
    firstprivate(rank, n_bytes, approx_factor, pattern_length, my_pattern, \
                 cuda_device_exists, gpu_job_size, buf) shared(local_matches)
            {
                rank = rank;
                n_bytes = n_bytes;
                gpu_job_size = gpu_job_size;
                pattern_length = pattern_length;
                buf = buf;

                // Overall idea: if there is a cuda device, omp threads take on
                // just the second half of the workload + "ghost cells"
                int starting_point = cuda_device_exists
                                         ? (gpu_job_size - (pattern_length + 1))
                                         : 0;

                approx_factor = approx_factor;
                my_pattern = my_pattern;

                int j;

                int *column = (int *)malloc((pattern_length + 1) * sizeof(int));

                int chunk_size = ((n_bytes - approx_factor) - starting_point) /
                                 omp_get_num_threads();

#pragma omp for schedule(static, chunk_size)
                for (j = starting_point; j < n_bytes - approx_factor; j++) {
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
            }

            if (cuda_device_exists) {
                write_kernel_result(&device_result, device_result_address);
                local_matches += device_result;
            }

#if APM_DEBUG
            printf("Rank %d sending result: n_matches[%d] = %d\n", rank, tag,
                   local_matches);
#endif
            mpi_call_result =
                MPI_Send(&local_matches, 1, MPI_INT, 0, tag, MPI_COMM_WORLD);
            if (mpi_call_result != MPI_SUCCESS) {
                printf("MPI Error: %d\n", mpi_call_result);
                return 1;
            }
        }

#if APM_DEBUG
        printf("\n(Rank %d) Finished Loop\n", rank);
#endif
    }

    return 0;
}
