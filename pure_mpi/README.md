[<-- go back](../)

# Approximate Pattern Matching - Distributed Memory (MPI)


Approaches:

1. Distribution of the patterns over the MPI ranks
    ```
    Procedure followed :
    
    - Master Process broadcasts the input database with MPI_Broadcast
    - Master Process distributes the patterns to search with MPI_Scatterv
    - Worker processes send their local results to Master with MPI_Send
    - Master process collects results with MPI_Recv and then reports
    ```

2. Distribution of the processing of one pattern among the ranks