[<-- go back](https://github.com/linomp/INF560_APM_Final_Project)

# Hybrid - Shared & Distributed Memory (MPI + OpenMP)

Approaches:

1. First MPI-then OpenMP: distribute the patterns over the MPI ranks & spawn an OpenMP team within every MPI rank. [[Code](./src/apm_patterns_over_ranks.c)]
