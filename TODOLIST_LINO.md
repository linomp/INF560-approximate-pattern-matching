# TODOLIST FOR LINO

- [X] Check quickly commits messagges: ask me for any doubts.

- [X] Read carefully the new test cases file (renamed in Workflow): I re-wrote completely. You can find there the explanation of the new algorithm. I was really verbose, so you should understand everything.

- [X] Add in excel column "Active MPI Ranks", so we don't get crazy. Just Total_MPI_Ranks - 1.

- [X] Implement the model explained in test cases in the code. You already find the pseudo-code, so it should be easy. NOTE: The new workflow for multiple patterns work also for the case of only 1 pattern. Check it but I should be right (to understand better you could find useful to compare the number of threads lost and the ratioHardwareOptimizationApproachChosen). In this case in the code you can avoid to differentiate when we have 1 or multiple patterns (But leave the number of threads lost in excel file).

- [ ] Since I changed the data inside the big database, you should re-run the tests of big database and update the results in the excel. I changed the size since with both multiple threads and multiple ranks it was too little. Also we should make it future-proof for GPU.

- [ ] Run the new scripts I added (folder ranks_threads in both big and medium database) and update result in excel.

- [ ] Add some other hybrid scripts (ranks_threads) so we can be sure of the correctness of the algorithm. Useless to say, be sure to do this after have understood how things work reading the test cases.

- [ ] Implement GPU in your code in a branch or somewhere else. Merge after you finish.

# SUGGESTION

- [X] You may find useful what I do in my sync_all: you can automatically delete the folder in the bash script, so you don't get crazy every time.


- for additional examples: focus on representative cases (number > 2)