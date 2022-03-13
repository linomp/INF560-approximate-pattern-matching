# Test Cases

## Number of patterns

### 1 pattern

#### Lino

You are parallelizing patterns over MPI ranks. 
So if you have just one big pattern, you would have just one MPI rank dealing with the pattern.
If you have, for instance, 10 ranks, you would use just 1.
OpenMP works because you are splitting the database in pieces, and every thread is reading a piace of the database to search that pattern.

#### Paolo

I parallelize with MPI Ranks: the database is splitted over the ranks.
Every MPI rank searches for the pattern in its piece of database.
OpenMP is not active because there is just 1 pattern to search for.

### Multiple patterns

#### Lino

Better than before.
The number of patterns is = as the processes: this is the best scenario.
The number of patterns is > than the processes: less performance than =. Overhead due to round robin.
The number of pattern is < than the processes: some MPI ranks are unused (use of hardware is not optimized).

There is no correlation with the number of threads: all of them (at least of active MPI Ranks) are used to read the database.

#### Paolo

There is no correlation with the number of MPI ranks: all of them are used to read the database.

The number of patterns is = as the number of threads: this is the best case scenario. I am using all the hardware provided.
The number of patterns is > than the number of threads. Overhead due to round robin.
The number of pattern is < than the threads of threads. Some threads are not used (hardware is not optimized).

## Size of database

### Big database

#### Lino

Few threads: bad performances.
Many threads: good performances.

No correlation with processes.

#### Paolo

Few processes: bad performances.
Many processes: good performances.

No correlation with threads.

### Small database

#### Lino

Few threads: good use of hardware.
Many threads: bad use of hardware.

No correlation with processes.

#### Paolo

Few processes: good use of hardware.
Many processes: bad use of hardware.

No correlation with threads.

### Bonus: send the database

#### Lino

Send all the data to all the ranks.

#### Paolo

Can split the database in pieces and send every piece to each rank. (MPI Scatter?)

# Overviews

## Paolo - Database over ranks PREFERRING MPI RANKS

- Rank 0 reads the size of the database and reads the number of processes connected.
- Rank 0 divides the database for the number of processes - 1.
- It's useless to divide the database in more pieces, it would only create overhead.
- Rank 0 sends every piece at every rank.

OpenMP for searching pattern in parallel.


## Lino - Pattern over ranks PREFERRING THREADS

Patterns over ranks.
- We distribute the patterns over the MPI ranks & spawn an OpenMP team within every MPI rank.
- The number of OpenMP threads can be 1; in this case, the code behaves as in pure-mpi (work division across ranks, but sequential execution within each rank).
- If there are more omp threads available, each one takes a chunk of the "input database" and searches ocurrences of the pattern assigned to their parent MPI rank with the levenshtein function.
- No parallelization took place inside the levenshtein function itself.

# Combinations

## 1 pattern with big database

### Lino 

If you have 1 rank is good.
If you have more than one you would waste all the other ones.

### Paolo

With 1 rank I don't split the database. Threads are not used.
If I have more ranks better performance. Threads are not used.

### Conclusions

#### 1 MPI Rank

With 1 rank Paolo's code is like sequential programming: I don't split the database and I don't use threads since there is just 1 pattern.
With 1 rank Lino's code takes advantage of shared-memory parallelism. Threads are used in this case. This is the best approach.

#### Multiple MPI Ranks

##### Without GPU

L<sub>l</sub> Lino Loss = (MPI_Ranks - 1) * OMP_Threads
L<sub>p</sub> Paolo Loss = MPI_Ranks * (OMP_Threads - 1)
choose min (L<sub>l</sub>, L<sub>p</sub>)

##### With MPI Ranks on different nodes (GPU for each rank)

Paolo GPU Loss = MPI_Ranks
Lino GPU Loss = MPI_Ranks - 1

L<sub>p</sub> Paolo Loss = MPI_Ranks * (((OMP_THREADS-1)*2)-1)
L<sub>l</sub> Lino Loss = (MPI_Ranks - 1) * (OMP_Threads-1) * 2
choose min (L<sub>l</sub>, L<sub>p</sub>)

##### With some of MPI Ranks on the same node (GPU shared for each rank)

We cannot determine if the MPI ranks are on the same node, so we cannot modify our workflow to optimize the given hardware.
Some experiment will be performed but the model won't change.
Number of ranks larger than GPUs would result in round robin scheduling (if the user is not smart enough).

#### Example

##### 5 MPI Ranks

2 Threads per MPI.
Lino loses 4 x 2 = 8
Paolo loses 5 * (2-1)  = 5
Paolo wins

10 Threads per MPI
Lino loses 4 x 10 = 40
Paolo loses (10 - 1) * 5 = 45
Lino wins

##### 20 MPI Ranks

2 Threads per MPI.
Lino loses 19 x 2 = 38
Paolo loses 20 * (2-1)  = 20
Paolo wins

10 Threads per MPI
Lino loses 19 x 10 = 190
Paolo loses 20 x (10 - 1) = 180
Paolo wins

## Multiple patterns

ratioLino = MPI_Ranks/patterns:
ratioLino = 1

ratioPaolo = Threads/patterns
ratioPaolo = 1

[WITH GPU]
ratioPaolo = ((Threads - 1) * 2)/2 ????

2/4 = 0.5
4/2

if(ratioLino == 1 and ratioPaolo != 1){
    choose Lino
}
else if(ratioLino != 1 and ratioPaolo == 1){
    choose Paolo
}
else if (ratioLino < 1 and ratioPaolo > 1){
    choose ratioPaolo
}
else if (ratioLino > 1 and ratioPaolo < 1){
    choose ratioLino
}
else if(ratioLino < 1 and ratioPaolo < 1){
    // Should be max but depends on which round robin costs more
}
else if (ratioLino > 1 and ratioPaolo > 1){
    // Should be minimum but depends on the optimization
}
else if (ratioLino == 1 and ratioPaolo == 1)

MPI Ranks: 4
Threads: 32
Patterns: 16

ratioLino = MPI_Ranks/patterns = 4/16 = 0.25
ratioPaolo = threads/patterns = 32/16 = 2

MPI Ranks: 4
Threads: 4
Patterns: 4

ratioLino = MPI_Ranks/patterns = 1
ratioPaolo = threads/patterns = 1

5 patterns

Paolo:

Rank1: 12345 -> (Thread1, Thread2, Thread3, Thread4) they finish at same time -> After some time Thread5 searches for the last pattern
Rank2: 67890 -> (Thread1, Thread2, Thread3, Thread4) they finish at same time -> After some time Thread5 searches for the last pattern
Rank3: 1234 -> (Thread1, Thread2, Thread3, Thread4) they finish at same time -> After some time Thread5 searches for the last pattern
Rank4: 67890 -> (Thread1, Thread2, Thread3, Thread4) they finish at same time -> After some time Thread5 searches for the last pattern

Pattern is already in memory.

Lino:

Rank1: Pattern1: (Thread1, Thread2, Thread3, Thread4) search for pattern in all the database
Rank2: Pattern2: (Thread1, Thread2, Thread3, Thread4) search for pattern in all the database
Rank3: Pattern3: (Thread1, Thread2, Thread3, Thread4) search for pattern in all the database
Rank4: Pattern4: (Thread1, Thread2, Thread3, Thread4) search for pattern in all the database

Rank5: Pattern1: (Thread1, Thread2, Thread3, Thread4) search for pattern in all the database
The master sends the pattern through communication.