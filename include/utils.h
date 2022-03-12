#pragma once

char *read_input_file(char *filename, int *size);

#define MIN3(a, b, c) ((a) < (b) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c)))

#define TESTPERFORMANCE_NO_LEVENSHTEIN 1

int levenshtein(char *s1, char *s2, int len, int *column);