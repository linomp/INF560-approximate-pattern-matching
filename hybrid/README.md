[<-- go back](https://github.com/linomp/INF560_APM_Final_Project)

# Hybrid - Shared & Distributed Memory (MPI + OpenMP)

Approaches:

1. First MPI-then OpenMP: distribute the patterns over the MPI ranks & spawn an OpenMP team within every MPI rank. [[Code](./src/apm_patterns_over_ranks.c)]


## TO-DO: 
- Fix Issue: result not consistent across runs & number of openmp threads - sometimes correct, sometimes correct
    - probably due to "borders" of domain partitioning
- Profile the code (base case = 1 MPI task)
- Add OpenMP driven by profiling (currently added a parallel region but without any criteria)
- Apply OpenMP best practices: Lecture 6 slide # 66
