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

Criteria for deciding between the two approaches at runtime is documented in: [Test Cases](./Test%20Cases.md)


### TO-DO
- [ ] Create folder for tests & batch scripts
- [ ] add sequential to makefile (still actively used code)
- [ ] Take into consideration only 1 MPI Rank (fallback to sequential? or just report error? our min. is 2)