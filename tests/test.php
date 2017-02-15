<?php 
$host_list="127.0.0.1:2181,127.0.0.1:2182,127.0.0.1:2183";
$zh = distlock_connect($host_list,300000);
if($zh == NULL) {
  echo "connect failure\n";
  exit();
}

$lock_path="/account-locks/liuxingzhi";
$mutex = distlock_init($zh,$lock_path);
if($mutex == NULL) {
 echo "lock init failure\n";
 exit();
}

$islock = distlock_lock($mutex, 5);
if($islock == true)
  echo "Get Lock ok\n";
else
  echo "Get Lock failure\n";

sleep(60);

distlock_unlock($mutex);
distlock_free($mutex);
distlock_disconnect($zh);
?>

