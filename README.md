# INF560_APM_Final_Project

Paolo Calcagni & Lino Mediavilla

Parallelization of the [Approximate Pattern Matching](https://www.enseignement.polytechnique.fr/profs/informatique/Patrick.Carribault/INF560/TD/projects/INF560-projects-0.html#topic3) problem, using 3 different paradigms:

- Distributed Memory (MPI)
- Hybrid - Shared & Distributed Memory (MPI + OpenMP)
- Heterogeneous (MPI + OpenMP + CUDA)

## Approaches:

### 1. Patterns over ranks

- We distribute the patterns over the MPI ranks & spawn an OpenMP team within every MPI rank.
- The number of OpenMP threads can be 1; in this case, the code behaves as in pure-mpi (work division across ranks, but sequential execution within each rank).
- If there are more omp threads available, each one takes a chunk of the "input database" and searches ocurrences of the pattern assigned to their parent MPI rank with the levenshtein function.
- No parallelization took place inside the levenshtein function itself.

### 2. Database over ranks

- Rank 0 reads the size of the database and reads the number of processes connected.
- Rank 0 divides the database for the number of processes - 1.
- It's useless to divide the database in more pieces, it would only create overhead.
- Rank 0 sends every piece at every rank.
- Then OpenMP is used for searching pattern in parallel.

### Decision criteria

Criteria for deciding between the two approaches at runtime is documented in: [Workflow](./Workflow.md)

## Compiling & Running

To compile, at the root of this directory type:

`make`

To search and use a GPU, a flag is required:

`make USE_GPU_FLAG=-DUSE_GPU_FLAG`

_Note: the code falls-back to only MPI + Open MP in case of no GPU detected._

The executable can be run like this:

`OMP_NUM_THREADS=4 salloc -N 2 -n 3 mpirun ./apm_parallel 0 ./dna/small_chrY_x100.fa <pattern 1> <pattern 2>`

We provide a simple test script, run it with:

`bash scripts/basic_test.batch`

And it should give you an output like the following:

```
Running SEQUENTIAL
Approximate Pattern Mathing: looking for 6 pattern(s) in file ./dna/small_chrY_x100.fa w/ distance of 0
APM done in 4.111790 s
Number of matches for pattern <QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ>: 0
Number of matches for pattern <CACCCCCAAAATATAGATTCTTCCCCAATTTATGTCTGAAAACAGGACCC>: 4
Number of matches for pattern <CACCCCCAAAATATAGATTCTTCCCCAATTTATGTCTGAAAACAGGACCC>: 4
Number of matches for pattern <CACCCCCAAAATATAGATTCTTCCCCAATTTATGTCTGAAAACAGGACCC>: 4
Number of matches for pattern <CACCCCCAAAATATAGATTCTTCCCCAATTTATGTCTGAAAACAGGACCC>: 4
Number of matches for pattern <CACCCCCAAAATATAGATTCTTCCCCAATTTATGTCTGAAAACAGGACCC>: 4

Running PATTERNS_OVER_RANKS TEST
salloc: Granted job allocation 34357
Approximate Pattern Matching: looking for 6 pattern(s) in file ./dna/small_chrY_x100.fa w/ distance of 0

(Rank 0) - TOTAL TIME using 3 mpi_ranks and 4 omp_thread(s) per rank: 1.813988 s

Number of matches for pattern <QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ>: 0
Number of matches for pattern <CACCCCCAAAATATAGATTCTTCCCCAATTTATGTCTGAAAACAGGACCC>: 4
Number of matches for pattern <CACCCCCAAAATATAGATTCTTCCCCAATTTATGTCTGAAAACAGGACCC>: 4
Number of matches for pattern <CACCCCCAAAATATAGATTCTTCCCCAATTTATGTCTGAAAACAGGACCC>: 4
Number of matches for pattern <CACCCCCAAAATATAGATTCTTCCCCAATTTATGTCTGAAAACAGGACCC>: 4
Number of matches for pattern <CACCCCCAAAATATAGATTCTTCCCCAATTTATGTCTGAAAACAGGACCC>: 4
salloc: Relinquishing job allocation 34357
 
Running DB_OVER_RANKS TEST
salloc: Granted job allocation 34358
Approximate Pattern Mathing: looking for 6 pattern(s) in file ./dna/small_chrY_x100.fa w/ distance of 0

(Rank 0) - TOTAL TIME using 3 mpi_ranks and 4 omp_thread(s) per rank: 1.684821 s

Number of matches for pattern <QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ>: 0
Number of matches for pattern <CACCCCCAAAATATAGATTCTTCCCCAATTTATGTCTGAAAACAGGACCC>: 4
Number of matches for pattern <CACCCCCAAAATATAGATTCTTCCCCAATTTATGTCTGAAAACAGGACCC>: 4
Number of matches for pattern <CACCCCCAAAATATAGATTCTTCCCCAATTTATGTCTGAAAACAGGACCC>: 4
Number of matches for pattern <CACCCCCAAAATATAGATTCTTCCCCAATTTATGTCTGAAAACAGGACCC>: 4
Number of matches for pattern <CACCCCCAAAATATAGATTCTTCCCCAATTTATGTCTGAAAACAGGACCC>: 4
salloc: Relinquishing job allocation 34358
```