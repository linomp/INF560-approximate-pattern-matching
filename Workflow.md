# 1 pattern

## Overview 

### Patterns over Ranks (Lino)

Patterns are parallelized over MPI ranks. 
If there is just one big pattern, only one MPI rank would deal with the pattern. (If there are 10 ranks, for instance, only one rank would be used).
OpenMP threads work because they split the database in pieces: every thread reads a piece of the database to search that pattern.

With 1 rank and multiple threads, this code uses all the threads of the only available rank.
With multiple ranks and 1 thread, this code doesn't use multiple ranks and uses just the threads of one rank. It's behaves like the sequential program.

### Database over Ranks (Paolo)

The database is split over the ranks.
Every MPI rank searches for the pattern in its piece of database.
OpenMP threads are not active because there is just 1 pattern to search for.

With 1 rank and multiple threads, this code is like sequential programming: the database is not split into pieces and threads are not used since there is only 1 pattern.
With multiple ranks and 1 thread, the code uses all the MPI ranks to split the database and uses the only thread available to search for the pattern.

## Considerations

### Without GPU

L<sub>l</sub> Patterns over Ranks Lost Threads = (Active_MPI_Ranks - 1) * OMP_Threads

L<sub>p</sub> Database over Ranks Lost Threads = Active_MPI_Ranks * (OMP_Threads - 1)

choose min(L<sub>l</sub>, L<sub>p</sub>)

#### Example

##### 5 MPI Ranks

2 Threads
Patterns over Ranks loses 4 x 2 = 8
Database over Ranks loses 5 * (2-1)  = 5
Database over Ranks wins

10 Threads 
Patterns over Ranks loses 4 x 10 = 40
Database over Ranks loses (10 - 1) * 5 = 45
Patterns over Ranks wins

##### 20 MPI Ranks

2 Threads
Patterns over Ranks loses 19 x 2 = 38
Database over Ranks loses 20 * (2-1)  = 20
Database over Ranks wins

10 Threads
Patterns over Ranks loses 19 x 10 = 190
Database over Ranks loses 20 x (10 - 1) = 180
Database over Ranks wins

### With MPI Ranks on different nodes (GPU for each rank)

Patterns over Ranks GPU Lost = Active_MPI_Ranks - 1
Database over Ranks GPU Lost = Active_MPI_Ranks

L<sub>l</sub> Patterns over Ranks Threads Lost = (Active_MPI_Ranks - 1) * (OMP_Threads-1) * 2
L<sub>p</sub> Database over Ranks Lost = Active_MPI_Ranks * (((OMP_THREADS-1)*2)-1)
choose min (L<sub>l</sub>, L<sub>p</sub>)

### With some of MPI Ranks on the same node (GPU shared for each rank)

We cannot determine if the MPI ranks are on the same node, so we cannot modify our workflow to optimize the given hardware.
Some experiment will be performed but the model won't change.
Number of ranks greater than GPUs would result in round robin scheduling.

If the user is smart enough should allocate a MPI Rank for each node: in this way every MPI rank would have its own GPU and doesn't have to share it.

# Multiple patterns or 1 Pattern

This approach works well even if there is only 1 pattern.

## Overview

### Patterns over Ranks

- The number of patterns is = as the actual MPI ranks. There is no round robin. All the hardware is being used (Every MPI rank has a pattern. All the ranks will finish at the same time, after one iteration.).
- The number of patterns is > than the actual MPI ranks. There is round robin. If the execution time of every job is a dividend of the time slice, all the hardware is being used. Otherwise the use of the hardware is not optimized.
- The number of pattern is < than the actual MPI ranks: some MPI ranks are not used. The use of the hardware is not optimized.

There is no correlation with the number of threads: all of them (at least of active MPI Ranks) are used to read the database.

### Database over Ranks

The number of patterns is = as the number of threads. There is no round robin. All the hardware is being used.
The number of patterns is > than the number of threads. There is round robin. If the execution time of every job is a dividend of the time slice, all the hardware is being used. Otherwise the use of hardware is not optimized.
The number of pattern is < than the threads of threads. Some threads are not used. The use of the hardware is not optimized.

There is no correlation with the number of MPI ranks: all of them are used to read the database.

## Considerations

### Without GPU

dimensionOfJobsPatternsOverDatabase = Active_MPI_Ranks/patterns
dimensionOfJobsDatabaseOverDatabase = ThreadsPerRank/patterns

We can calculate HardwareOptimizationPatternsOverDatabase (ratioPatterns) and HardwareOptimizationDatabaseOverPatterns (ratioDatabase) in this way.
Assume x is dimensionOfJobsPatternsOverDatabase or dimensionOfJobsDatabaseOverDatabase.

```

    if(x < 1){
        while(x <= 1){
            x = x * 2;
        }
        ratioHardwareOptimizationApproachChosen = x % 1;
    }
    else if(x >= 1 and x < 2){
        ratioHardwareOptimizationApproachChosen = x % 1;
    }
    else{
        ratioHardwareOptimizationApproachChosen = x;
    }
   
```

```

    if(ratioPatterns == 0 && ratioDatabase == 0){
        random(DatabaseOverRanks, PatternsOverRanks)
        // It's the same. Both of the approaces use the hardware at its maximum capacity.
    }
    else{
        if(Minimum(ratioPatterns, ratioDatabase) == ratioPatterns){ // We choose the approach who optimize better the use of hardware.
            choose PatternsOverRanks;
        }
        else{
            choose DatabaseOverRanks;
        }
        
    }

```

#### Examples

##### Theoretical Examples

###### Example 1

Let's assume we have 2 MPI_ranks, 4 threads, 4 patterns and database with 100 characters.

dimensionOfJobsPatternsOverDatabase = Active_MPI_Ranks/patterns = 2/4 = 0.5
dimensionOfJobsDatabaseOverDatabase = ThreadsPerRank/patterns = 4/4 = 1

ratioPatterns = 0
ratioDatabase = 0

Both of the approaches maximize the usage of hardware: let's see why.

PatternsOverRanks spreads the first 2 patterns over the 2 MPI ranks and when they finish, it spreads the 2 left patterns over the ranks again.
Each MPI_Rank split the database in the number of threads, 4 in this case. 100/4 = 25. Each of the 4 threads has to analyze 25 characters.
Suppose that the thread needs 1 second for each character. It means that each thread needs 25 seconds.
After 25 seconds the 2 threads finish; a second round has to be done.
Other 25 seconds will pass.
The total execution time will be then 50 seconds.

DatabaseOverRanks will spread the database over ranks, 2 in this case. 100/2 = 50. Every rank has 50 characters of the database.
Each MPI_Rank has 4 threads in this case.
Each thread will search in its piece of database: 50 characters.
Each thread will finish in 50 seconds.
After both of the threads will finish after 50 seconds, there is no need for a second round.
    
As we can see, the execution time is the same.

###### Example 2

Let's assume we have 4 MPI_ranks, 5 threads, 6 patterns and database with 100 characters.

dimensionOfJobsPatternsOverDatabase = Active_MPI_Ranks/patterns = 4/6 = 0.66
dimensionOfJobsDatabaseOverDatabase = ThreadsPerRank/patterns = 5/6 = 0.83

ratioPatterns = 0.32
ratioDatabase = 0.66

PatternsOverRanks will spread the first 4 patterns over the 4 MPI ranks and when they finish, it will spread the 2 left patterns over the ranks again.
Each MPI_Rank split the database in the number of threads, 5 in this case. 100/5 = 20. Each of the 5 threads has to analyze 20 characters.
Suppose that the thread needs 1 second for each character. It means that each thread needs 20 seconds.
After 20 seconds the 5 threads finish; a second round has to be done.
2 left pattern are spread over the ranks.
The total execution time will be then 40 seconds.

DatabaseOverRanks will spread the database over ranks, 4 in this case. 100/4 = 25. Every rank has 25 characters of the database.
Each MPI_Rank 5 threads in this case.
Each thread will search in its piece of database: 25 characters.
Each thread will finish in 25 seconds.
One pattern still left.
So the program will finish in 50 seconds.

As we can see, both of them are not using all the hardware; however, patterns over ranks finishes before.

###### Example 3

Let's assume we have 11 MPI_ranks, 5 threads, 6 patterns and database with 100 characters.

dimensionOfJobsPatternsOverRanks = Active_MPI_Ranks/patterns = 11/6 = 1.83
dimensionOfJobsDatabaseOverRanks = ThreadsPerRank/patterns = 5/6 = 0.83

ratioPatterns = 1.83
ratioDatabase = 1.66
    
PatternsOverRanks spread the first 6 patterns over the 11 MPI ranks.
Each MPI_Rank split the database in the number of threads, 5 in this case. 100/5 = 20. Each of the 5 threads has to analyze 20 characters.
Suppose that the thread needs 1 second for each character. It means that each thread needs 20 seconds.
After 20 seconds the 5 threads finish.
The total time will be 20 seconds in total.

RatioDatabase will spread the database over ranks, 11 in this case.. 100/11 = 9.09 Every rank has 9.09 characters of the database.
Each MPI_Rank has 5 threads.
Each thread searches in its piece of database: 9.09 characters.
After 9.09 seconds all thread finish but 1 pattern still left.
The total time will be 18.18 seconds.

##### Practical Examples
    
###### Example 1

Let's assume we have 4 MPI_ranks, 3 threads, 6 patterns and database with 100 characters.

dimensionOfJobsPatternsOverRanks = Active_MPI_Ranks/patterns = 4/6 = 0.6
dimensionOfJobsDatabaseOverRanks = ThreadsPerRank/patterns = 3/6 = 0.5

ratioPatterns = 0.2
ratioDatabase = 0

Execution time of Patterns over Ranks: 54 seconds
Execution time of Database over Ranks: 41 seconds

###### Example 2

Same input of theoretical example 2.

Let's assume we have 4 MPI_ranks, 5 threads, 6 patterns and database with 100 characters.

dimensionOfJobsPatternsOverRanks = Active_MPI_Ranks/patterns = 4/6 = 0.6
dimensionOfJobsDatabaseOverRanks = ThreadsPerRank/patterns = 5/6 = 0.8

ratioPatterns = 0.2
ratioDatabase = 0.6

Execution time of Patterns over Ranks: 30.42 seconds
Execution time of Database over Ranks: 43.24 seconds

###### Example 3

Let's assume we have 9 MPI_ranks, 5 threads, 6 patterns and database with 100 characters.

dimensionOfJobsPatternsOverRanks = Active_MPI_Ranks/patterns = 9/6 = 1.5
dimensionOfJobsDatabaseOverRanks = ThreadsPerRank/patterns = 5/6 = 0.83

ratioPatterns = 0.5
ratioDatabase = 0.66

Execution time of Patterns over Ranks: 15.93 seconds
Execution time of Database over Ranks: 19.30 seconds

###### Example 4

Same input of theoretical example 3.

Let's assume we have 11 MPI_ranks, 5 threads, 6 patterns and database with 100 characters.

dimensionOfJobsPatternsOverRanks = Active_MPI_Ranks/patterns =  11/6 = 1.83
dimensionOfJobsDatabaseOverRanks = ThreadsPerRank/patterns = 5/6 = 0.83

ratioPatterns = 0.83
ratioDatabase = 0.66

Execution time of Patterns over Ranks: 15.92 seconds
Execution time of Database over Ranks: 15.88 seconds

### With GPU

//TODO To validate following reasoning

dimensionOfJobsPatternsOverRanks = Active_MPI_Ranks/patterns
dimensionOfJobsDatabaseOverRanks = ((ThreadsPerRank-1) * 2)/patterns

Let's assume we have 4 Active_MPI_ranks, 4 threads, 6 patterns and database with 100 characters, 1 GPU.

PatternsOverRanks spreads the first 4 patterns over the 4 MPI ranks and when they finish, it will spread the 2 left patterns over the ranks again.
Each MPI_Rank split the database in (threads-1)*2 = (4-1)*2= 6. 100/6 = 16,5 characters.
GPU and 3 threads will search the pattern in 16.5 characters.
After 16.5 seconds 3 threads and GPU will finish.
A second round has to be done: 2 left pattern will spread on 2 Mpi_Ranks (out of 4).
The total execution time will be then 33 seconds.

dimensionOfJobsPatternsOverRanks = Active_MPI_Ranks/patterns = 4/6 = 0.6
ratioDatabase = 0.2

DatabaseOverRanks spreads the database over ranks, 4 in this case. 100/4 = 25. Every rank has 25 characters of the database.
Each MPI_Rank looks for (threads-1)*2 = (4-1)*2 = 6 patterns.
GPU and 3 threads will look for 6 patterns.
Each thread searches in its piece of database: 25 characters.
After 25 seconds the threads will finish. There is no need for a second round.
The total execution time is 25 seconds.

dimensionOfJobsDatabaseOverRanks = ((ThreadsPerRank-1) * 2)/patterns = (4-1)*2/6 = 6/6 = 1
ratioDatabase = 0


