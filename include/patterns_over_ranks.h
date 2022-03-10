/**
 * APPROXIMATE PATTERN MATCHING
 *
 * INF560
 *
 * Hybrid Approach #1: distribute the patterns over the MPI ranks;
 *                     parallelize the processing of a pattern within a rank
 *
 *
 */
#pragma once

int patterns_over_ranks_hybrid(int argc, char **argv, int rank, int world_size);