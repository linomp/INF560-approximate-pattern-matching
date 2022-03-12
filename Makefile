SRC_DIR=src
HEADER_DIR=include
OBJ_DIR=obj

MPI_CC=mpicc
CFLAGS=-O3 -I$(HEADER_DIR) -Wall -fopenmp -DAPM_INFO
LDFLAGS=

SRC= main.c patterns_over_ranks.c database_over_ranks.c utils.c

OBJ= $(OBJ_DIR)/patterns_over_ranks.o $(OBJ_DIR)/database_over_ranks.o $(OBJ_DIR)/main.o $(OBJ_DIR)/utils.o

all: $(OBJ_DIR) main utils patterns_over_ranks database_over_ranks

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	$(MPI_CC) $(CFLAGS) -c -o $@ $^

utils:$(OBJ)
	$(MPI_CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

database_over_ranks:$(OBJ)
	$(MPI_CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

patterns_over_ranks:$(OBJ)
	$(MPI_CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

main: $(OBJ)
	$(MPI_CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

clean:
	rm -f main patterns_over_ranks database_over_ranks $(OBJ) ; rmdir $(OBJ_DIR)
