# You Spin Me Round Robin
Nicholas Callen
405739681

Implementation of a round-robin scheduler in C. Before running any below command, make sure to be in the folder with the C file by using cd/(Your Filepath)/.../...

## Building

```shell
make
```
Using make while in the directory will create an executable for rr.

## Running

cmd for running ./rr
```shell
./rr processes.txt 3
```
Runs the executable using processes.txt file as input with a quantum of 3.
./rr (data file) (quantum)

results
```shell
Average waiting time: 67.75
Average response time: 3.25

```
Average times depend on the inputs used as well as the quantum time.
Average waiting time: X
Average response time: Y

## Cleaning up

```shell
make clean
```
Make clean will remove all binary files.
