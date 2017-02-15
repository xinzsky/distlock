#include <stdio.h>
#include <stdlib.h>
#include <zookeeper.h>

void watcher(zhandle_t *zh, int type, int state, const char *path, void *watcherCtx)
{
  printf("*********************Event Trigger********************.\n");
  return;
}


int main()
{
  zhandle_t *zh;
  struct Stat stat;
  int ret;

  zh = zookeeper_init("127.0.0.1:2181,127.0.0.1:2182,127.0.0.1:2183",NULL,300000,0,NULL,0);
  if(!zh) {
    printf("zookeeper_init() error.\n");
    return 0;
  }
  
  ret = zoo_wexists(zh, "/test_watcher", watcher, NULL, &stat);
  if(ret == ZOK)
    printf("zoo_wexists() ok.\n");
  else if(ret == ZNONODE)
    printf("znode[/test_watcher] inexist.\n");
  else 
    printf("error.\n");
  
  printf("wait watcher trigger....\n");
  sleep(60);  
  zookeeper_close(zh);
  return 1;
}

