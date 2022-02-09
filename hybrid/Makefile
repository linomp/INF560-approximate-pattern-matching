SRC_DIR=src
HEADER_DIR=include
OBJ_DIR=obj

MPI_CC=mpicc
CFLAGS=-O3 -I$(HEADER_DIR) -Wall -fopenmp
LDFLAGS=

SRC= apm_patterns_over_ranks.c

OBJ= $(OBJ_DIR)/apm_patterns_over_ranks.o

all: $(OBJ_DIR) apm_patterns_over_ranks

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	$(MPI_CC) $(CFLAGS) -c -o $@ $^

apm_patterns_over_ranks:$(OBJ)
	$(MPI_CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

clean:
	rm -f apm_patterns_over_ranks $(OBJ) ; rmdir $(OBJ_DIR)
