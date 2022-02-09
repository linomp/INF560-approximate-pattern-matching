[<-- go back](../README.md)

# Approximate Pattern Matching - Distributed Memory (MPI)


Approaches:

1. Distribution of the patterns over the MPI ranks
    ```
    Procedure followed :
    
    - MPI_Broadcast the input database
    - MPI_Scatterv the patterns to search
    - MPI_Gather in the Master process
    - Master process reports results
    ```

2. Distribution of the processing of one pattern among the ranks