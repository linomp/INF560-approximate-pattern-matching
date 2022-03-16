
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

// CUDA-C includes
#include <cuda.h>
#include <cuda_runtime.h>

#include <cstdio>

#define DEBUG_CUDA 0
#define TESTPERFORMANCE_NO_LEVENSHTEIN 1

#define MIN3(a, b, c) \
    ((a) < (b) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c)))

__global__ void searchPattern(d_buf, n_bytes, pattern, nb_patterns, lastPatternAnalyzedByGPU, sizePatterns, numbersOfMatch, indexFinishMyPieceWithoutExtra, myRank, numberProcesses, indexStartMyPiece, approx_factor){

    // I analyze the second half of the patterns
    // for (i = 0; i < lastPatternAnalyzedByGPU; i++) {
    if(i < lastPatternAnalyzedByGPU){
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

        column = (int *) malloc((size_pattern + 1) * sizeof(int));
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
            // distance = levenshtein(pattern[i], &buf[r], size, column);

            unsigned int x, y, lastdiag, olddiag;

#pragma unroll
            for (y = 1; y <= size; y++) {
                column[y] = y;
            }
#pragma unroll
            for (x = 1; x <= size; x++) {
                column[0] = x;
                lastdiag = x - 1;
                for (y = 1; y <= size; y++) {
                    olddiag = column[y];
                    column[y] = MIN3(
                            column[y] + 1, column[y - 1] + 1,
                            lastdiag + (pattern[y - 1] == (&buf[j])[x - 1] ? 0 : 1));
                    lastdiag = olddiag;
                }
            }

            distance = column[size];

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


extern "C" int * searchPatternsInPieceDatabase(char *buf, int n_bytes, char **pattern, int nb_patterns, int lastPatternAnalyzedByGPU, int *sizePatterns, int indexFinishMyPieceWithoutExtra, int myRank, int numberProcesses, int indexStartMyPiece, int approx_factor){

    // Allocate arrays in device memory

    int numbersOfMatch[nb_patterns];

    char *d_buf;
    cudaMalloc(&d_buf, n_bytes);
    cudaMemcpy(d_buf, buf, n_bytes, cudaMemcpyHostToDevice);

    int *d_sizePatterns;
    cudaMalloc(&d_sizePatterns, nb_patterns * sizeof(int));

    cudaMemcpy(d_sizePatterns, sizePatterns, nb_patterns, cudaMemcpyHostToDevice);

    int *d_numbersOfMatch;
    cudaMalloc(&d_numbersOfMatch, nb_patterns * sizeof(int));

#if DEBUG_CUDA
    printf("DEBUG_CUDA: Starting memory transfers...\n");
#endif

    char *d_pattern;
    cudaMalloc(&d_pattern, nb_patterns * sizeof(char *));

    // Allocando spazio per pattern
    for(int i = 0; i < nb_patterns; i++){
        char * patternInUse = &d_pattern[i];
        cudaMalloc(&patternInUse, sizePatterns[i] * sizeof(char));
        cudaMemcpy(&d_pattern[i], pattern[i], sizePatterns[i], cudaMemcpyHostToDevice);
    }

    // Initialize array
    for(int i = 0; i < nb_patterns; i++){
        d_numbersOfMatch = 0;
    }

    int sizeGrid = 256;
    int sizeBlocks = 1;

    searchPattern<<<sizeGrid, sizeBlocks>>>(d_buf, n_bytes, pattern, nb_patterns, lastPatternAnalyzedByGPU, sizePatterns, numbersOfMatch, indexFinishMyPieceWithoutExtra, myRank, numberProcesses, indexStartMyPiece, approx_factor);

    cudaMemcpy(numbersOfMatch, d_numbersOfMatch, nb_patterns * sizeof(int),
               cudaMemcpyDeviceToHost);

    return numbersOfMatch;

}
