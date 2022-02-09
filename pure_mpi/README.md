[<-- go back](https://github.com/linomp/INF560_APM_Final_Project)

# Approximate Pattern Matching - Distributed Memory (MPI)


Approaches:

1. Distribution of the patterns over the MPI ranks. [[Code](./src/apm_patterns_over_ranks.c)]
    ```
    Procedure followed :
    
    - Master Process broadcasts the input database with MPI_Broadcast
    - Master Process distributes the patterns to search (simple MPI_Send/Recv scheme)
    - Worker processes send their local results to Master with MPI_Send
    - Master process collects results with MPI_Recv and then reports

    * limitation: Currently capable of processing exactly 1 process per pattern (no round-robin scheme implemented)
    ```

2. Distribution of the processing of one pattern among the ranks