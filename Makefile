SRC_DIR=src
HEADER_DIR=include
OBJ_DIR=obj

MPI_CC=mpicc
CFLAGS=-O3 -I$(HEADER_DIR) -Wall -fopenmp -DAPM_INFO
LDFLAGS=

SRC= main.c patterns_over_ranks.c

OBJ= $(OBJ_DIR)/patterns_over_ranks.o $(OBJ_DIR)/main.o

all: $(OBJ_DIR) main patterns_over_ranks

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	$(MPI_CC) $(CFLAGS) -c -o $@ $^

patterns_over_ranks:$(OBJ)
	$(MPI_CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

main: $(OBJ)
	$(MPI_CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

clean:
	rm -f main patterns_over_ranks $(OBJ) ; rmdir $(OBJ_DIR)
