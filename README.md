# SAHH_BPP.c

This is the main file for this project. It contains the implementation of a Simulated Annealing algorithm which plays the role of a selection hyper heuristic framework to solve the bin packing problem.

## Usage

To use this code, you can compile it using a C compiler. Here are the steps:

1. Open a terminal or command prompt.
2. Navigate to the directory where the `AIM_CW.c` file is located.
3. Compile the code by running the following command:
   
      ```gcc -std=c99 -lm SAHH_BPP.c -o SAHH_BPP```
5. Run the compiled program by executing the following command:
   
      ```./SAHH_BPP -s data_file -o solution_file -t max_time```
   
   where
   data_file is one of the problem instance files (binpack1.txt / binpack3.txt / binpack11.txt)
   max_time is the maximum time permitted for a single run of this algorithm
   solution_file is the file for the output of the solutions
