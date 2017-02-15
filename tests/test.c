#include <stdio.h>
#include <stdlib.h>
#include "zookeeper.h"
#include "zoo_lock.h"

// gcc -o test test.c ../zoo_lock.c -g -DTHREADED -I /usr/local/include/zookeeper -I ../ -lzookeeper_mt

int main(int argc,char *argv[])
{
  zhandle_t *zh;
  zkr_lock_mutex_t mutex;
  char *lock_path;

  if(argc == 2)
    lock_path = argv[1];
  else
    lock_path = "/account-locks/liuxingzhi";

  zh = zookeeper_init("127.0.0.1:2181,127.0.0.1:2182,127.0.0.1:2183",NULL,300000,0,NULL,0);
  if(!zh) {
    printf("zookeeper_init() error.\n");
    return 0;  
  }

  if(zkr_lock_init(&mutex, zh, lock_path, &ZOO_OPEN_ACL_UNSAFE) != 0) {
    printf("zkr_lock_init() error.\n");
    zookeeper_close(zh);
    return 0;
  }

  if(zkr_lock_lock(&mutex, 5) == 0) { 
    printf("lock failure.\n");
  } else {
    printf("lock ok.\n");
    sleep(10);
  }
  
  if(zkr_lock_unlock(&mutex) != 0) {
    printf("unlock failure.\n");
    zkr_lock_destroy(&mutex);
    zookeeper_close(zh);
    return 0;
  }
  printf("unlock ok.\n");
  
  zkr_lock_destroy(&mutex);
  zookeeper_close(zh);
  return 1;
}
