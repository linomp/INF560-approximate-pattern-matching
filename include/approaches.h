

// The hybrid approaches implemented:
int patterns_over_ranks_hybrid(int argc, char **argv, int rank, int world_size,
                               int cuda_device_exists);  // Lino
int database_over_ranks(int argc, char **argv, int myRank,
                        int numberProcesses, int cuda_device_exists);  // Paolo