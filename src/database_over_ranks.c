#include <mpi.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "approaches.h"
#include "utils.h"

#define DEBUG 0
#define DEBUGPIECEREAD 0
#define DEBUGCHARACTERS 0
#define DEBUGBYTEOPENMP 0
#define DEBUGOPENMPPOINTERS 0

int database_over_ranks(int argc, char **argv, int myRank,
                        int numberProcesses) {
    char **pattern;
    char *filename;
    int approx_factor = 0;
    int nb_patterns = 0;
    int i, j;
    char *buf;
    struct timeval t1, t2;
    double duration;
    int n_bytes;
    int *n_matches;

#if DEBUG
#pragma omp parallel
    {
        printf("Hello MPI %d (out of %d) & OpenMP %d (out of %d)\n", myRank,
               numberProcesses, omp_get_thread_num(), omp_get_num_threads());
    }
#endif

    // Check number of arguments
    if (argc < 4) {
        printf(
            "Usage: %s approximation_factor "
            "dna_database pattern1 pattern2 ...\n",
            argv[0]);
        return 1;
    }

    // Get the distance factor
    approx_factor = atoi(argv[1]);

    // Grab the filename containing the target text
    filename = argv[2];

    // Get the number of patterns that the user wants to search for
    nb_patterns = argc - 3;

    // Fill the pattern array
    pattern = (char **)malloc(nb_patterns * sizeof(char *));
    if (pattern == NULL) {
        fprintf(stderr, "Unable to allocate array of pattern of size %d\n",
                nb_patterns);
        return 1;
    }

    // I don't parallelize this for with OpenMP since it's already very fast in
    // sequential programming Grab the patterns from command line
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

#if DEBUG
    printf("Rank MPI %d. I read the patterns.\n", myRank);
#endif

    // I am rank 0
    if (myRank == 0) {
        printf(
            "Approximate Pattern Mathing: "
            "looking for %d pattern(s) in file %s w/ distance of %d\n",
            nb_patterns, filename, approx_factor);

        // Read the database
        buf = read_input_file(filename, &n_bytes);
        if (buf == NULL) {
            return 1;
        }

        // Allocate the array of matches
        n_matches = (int *)malloc(nb_patterns * sizeof(int));
        if (n_matches == NULL) {
            fprintf(stderr, "Error: unable to allocate memory for %ldB\n",
                    nb_patterns * sizeof(int));
            return 1;
        }

        // Timer start
        gettimeofday(&t1, NULL);

        int size_database = n_bytes;
        int numberPiecesDatabase =
            numberProcesses -
            1;  // Number of pieces we should divide the database into. The
                // number of processes less the rank 0.
        int size_piece;

        if (numberProcesses > 1) {
            size_piece = (size_database / numberPiecesDatabase);
        } else {
            // TODO If there is just one process, it does all the job.
            printf("There is just one process.");
            return 0;
        }

#if DEBUG
        printf(
            "Rank 0. Size of the database: %d. Number of pieces of database: "
            "%d. Size of a piece: %d\n",
            size_database, numberPiecesDatabase, size_piece);
#endif

        // Rank 0 send to other ranks the index and end of their own pieces.
        for (j = 1; j < numberProcesses; j++) {
            int indexStartMyPiece, indexFinishMyPieceWithoutExtra;

            // If I am the last rank I take all the elements from my
            // startingPoint until the end of the database. If the division of
            // the database had a remainder (left characters) I'll take it.
            if (j == numberProcesses - 1) {
                indexStartMyPiece = (j - 1) * size_piece;
                indexFinishMyPieceWithoutExtra = size_database;
            } else {
                indexStartMyPiece = (j - 1) * size_piece;
                indexFinishMyPieceWithoutExtra =
                    indexStartMyPiece +
                    size_piece;  // I don't add here the extra (size_pattern -
                                 // 1) but I'll do receiver-side depending on
                                 // the pattern (and just for the ranks which
                                 // are not the last one).
            }

            int info[2];
            info[0] = indexStartMyPiece;
            info[1] = indexFinishMyPieceWithoutExtra;

            // As Rank 0, I send to the rank the info about his own piece
            MPI_Send(info, 2, MPI_INT, j, 0, MPI_COMM_WORLD);
#if DEBUG
            printf("Rank 0. I sent to the rank %d the info.\n", j);
#endif
        }

        // Initialize the number of matches to 0
        for (i = 0; i < nb_patterns; i++) {
            n_matches[i] = 0;
        }

        // Check each pattern one by one
        for (i = 0; i < nb_patterns; i++) {
            // For each pattern I wait the answer from all the ranks involved.
            for (j = 1; j < numberProcesses; j++) {
                int numberMatches;
                MPI_Status status;
                MPI_Recv(&numberMatches, 1, MPI_INT, MPI_ANY_SOURCE, i,
                         MPI_COMM_WORLD, &status);

#if DEBUG
                printf(
                    "Rank 0. I have received number of matches of pattern %d "
                    "from rank %d: %d\n",
                    i, status.MPI_SOURCE, numberMatches);
#endif

                n_matches[i] = n_matches[i] + numberMatches;
            }

#if DEBUG
            printf("Rank 0. Research finished for the patterns.\n");
#endif
        }

        /* Timer stop and print it */
        gettimeofday(&t2, NULL);
        duration = (t2.tv_sec - t1.tv_sec) + ((t2.tv_usec - t1.tv_usec) / 1e6);
        printf(
            "\n(Rank %d) - TOTAL TIME using %d mpi_ranks and %d omp_thread(s) "
            "per rank: %f s\n\n",
            myRank, numberProcesses, atoi(getenv("OMP_NUM_THREADS")), duration);

        // Print the results
        for (i = 0; i < nb_patterns; i++) {
            printf("Number of matches for pattern <%s>: %d\n", pattern[i],
                   n_matches[i]);
        }
    }

    // If I am not the rank 0
    else {
        // Read the database
        buf = read_input_file(filename, &n_bytes);
        if (buf == NULL) {
            return 1;
        }

        int info[2];  // 1° element: indexStartMyPiece. 2° element:
                      // indexFinishMyPieceWithoutExtra.
        MPI_Status status;
        MPI_Recv(&info, 2, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD,
                 &status);
#if DEBUG
        printf("Rank %d. I received info from rank 0.\n", myRank);
#endif

        int indexStartMyPiece = info[0];
        int indexFinishMyPieceWithoutExtra =
            info[1];  // Without extra to recognize words between pieces.

        // Initialize array where the threads of openMP will store the results.
        int numbersOfMatch[nb_patterns];
        for (i = 0; i < nb_patterns; i++) {
            numbersOfMatch[i] = 0;
        }

        // The implementation is correct. However, I don't notice the
        // improvements of performance that I was expecting.
#pragma omp parallel default(none) private(i)                                \
    firstprivate(indexFinishMyPieceWithoutExtra, indexStartMyPiece, n_bytes, \
                 approx_factor, nb_patterns, numberProcesses, myRank)        \
        shared(buf, pattern, stderr, ompi_mpi_comm_world, ompi_mpi_int,      \
               numbersOfMatch)
        {
#pragma omp for schedule(static)

            for (i = 0; i < nb_patterns; i++) {
                double timestampStart;
                double timestampFinish;

#if DEBUG
                printf(
                    "----- MPI %d (out of %d) & OpenMP %d (out of %d). Started "
                    "to analize pattern n° %d.\n",
                    myRank, numberProcesses, omp_get_thread_num(),
                    omp_get_num_threads(), i);
#endif

                int size_pattern = strlen(pattern[i]);
                int *column;

                column = (int *)malloc((size_pattern + 1) * sizeof(int));
                if (column == NULL) {
                    fprintf(
                        stderr,
                        "Error: unable to allocate memory for column (%ldB)\n",
                        (size_pattern + 1) * sizeof(int));
                    // return 1;
                }

                // If I am not the last rank I should take in consideration
                // extra characters from the next piece: in this way I don't
                // miss words which are placed between two pieces. If am the
                // last rank I don't take extra characters as the other ranks
                // since the file is finished.
                int indexFinishMyPieceWithExtra =
                    indexFinishMyPieceWithoutExtra;
                if (myRank != numberProcesses - 1) {
                    indexFinishMyPieceWithExtra += size_pattern - 1;
                }

#if DEBUG
                printf(
                    "Rank %d. I received the info from rank 0. Start index: "
                    "%d. Finish index: %d\n",
                    myRank, indexStartMyPiece, indexFinishMyPieceWithoutExtra);
                printf("Rank %d. Final index updated: %d.\n", myRank,
                       indexFinishMyPieceWithExtra);
#endif

#if DEBUGPIECEREAD
                printf("Rank %d: I will read the following text:\n", myRank);
                int j;
                for (j = indexStartMyPiece;
                     j < indexFinishMyPieceWithExtra - approx_factor; j++) {
                    printf("%c", buf[j]);
                }
                printf("\n");
#endif

                // Traverse the input data up to the end of the file
                n_bytes = indexFinishMyPieceWithExtra;

#if DEBUG
                printf(
                    "----- MPI %d (out of %d) & OpenMP %d (out of %d). Index "
                    "Start: %d. Index finish: %d.\n",
                    myRank, numberProcesses, omp_get_thread_num(),
                    omp_get_num_threads(), indexStartMyPiece, n_bytes);
#endif

                timestampStart = omp_get_wtime();

                // It's not possible to parallelize with OpenMP this for since
                // the cycles are interconnected.
                int r;
                for (r = indexStartMyPiece; r < n_bytes - approx_factor; r++) {
#if DEBUGBYTEOPENMP
                    printf(
                        "MPI %d (out of %d) & OpenMP %d (out of %d). I am "
                        "analyzing byte %d for pattern %d\n",
                        myRank, numberProcesses, omp_get_thread_num(),
                        omp_get_num_threads(), r, i);

#endif

#if DEBUGCHARACTERS
                    printf("Rank %d. I read the character: %c \n", myRank,
                           buf[j]);
#endif

                    int distance = 0;
                    int size;
                    size = size_pattern;
                    if (n_bytes - r < size_pattern) {
                        size = n_bytes - r;
                    }

#if DEBUGOPENMPPOINTERS
                    printf(
                        "Pattern: %p. Buf: %p. Size: %p. Columns: %p.\ni "
                        "address: %p. i value: %d. j address: %p. j value: %d "
                        "\n",
                        &pattern, &buf[j], &size, &column, &i, i, &j, j);
#endif
                    distance = levenshtein(pattern[i], &buf[r], size, column);

                    if (distance <= approx_factor) {
                        numbersOfMatch[i] += 1;

#if DEBUG
                        printf("Rank %d. MATCH FOUND! \n", myRank);
#endif
                    }
                }
                timestampFinish = omp_get_wtime();

#if DEBUG
                double elapsedTime = timestampFinish - timestampStart;
                printf("Time elapsed for a thread: %g.\n", elapsedTime);
#endif
                free(column);
            }
        }

        // I send the result of the matches of every pattern to rank 0
        for (i = 0; i < nb_patterns; i++) {
            int numberToSend = numbersOfMatch[i];
            MPI_Send(&numberToSend, 1, MPI_INT, 0, i, MPI_COMM_WORLD);
#if DEBUG
            printf("Rank %d (out of %d). I sent the data of pattern %d\n",
                   myRank, numberProcesses, i);
#endif
        }
    }
    return 0;
}