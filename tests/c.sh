#!/bin/bash
#test.c
gcc -o test test.c ../zoo_lock.c -g -DTHREADED -I /usr/local/include/zookeeper -I ../ -lzookeeper_mt

#test_mt
gcc -o test_mt test_mt.c -g -DTHREADED -I /usr/local/include/zookeeper -lzookeeper_mt 

