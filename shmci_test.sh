#!/bin/sh

echo "Starting shmci test..."
echo
if ls shmci_test1 >> /dev/null
then
  echo "Found test1, executing..."
  ./shmci_test1
else
  echo "Cannot find test1, exiting"
  exit
fi
if [ $? -ne 0 ]
then
  echo "test1 failed, exiting"
  exit
else
  echo "test1 successful"
fi
echo

if (ls shmci_test2_0 >> /dev/null)&&(ls shmci_test2_1 >> /dev/null)
then
  echo "Found test2, executing..."
  ./shmci_test2_1 2>>log&
  sleep 1
  ./shmci_test2_0 2>>log&
  ./shmci_test2_0 2>>log&
  ./shmci_test2_0 2>>log&
  sleep 1
  ./shmci_test2_0 2>>log&
  sleep 2
  echo "."
  ./shmci_test2_0 2>>log&
  sleep 3
  ./shmci_test2_0 2>>log&
  ./shmci_test2_0 2>>log
  echo "."
  sleep 10
  echo "."
  sleep 5
  echo "."
else
  echo 'Cannot find test2, exiting'
  exit
fi
j=`cat log | grep -c Error`
rm log
if [ $j -ne 0 ]
then
  echo 'test2 failed, exiting'
  exit
else
  echo 'test2 successful'
fi
echo

if (ls shmci_test3_0 >> /dev/null)&&(ls shmci_test3_1 >> /dev/null)
then
  echo "Found test3, executing..."
  (./shmci_test3_0 2>>log&)&&(./shmci_test3_1 2>>log)
else
  echo "Cannot find test3, exiting"
  exit
fi
j=`cat log | grep -c Error`
rm log
if [ $j -ne 0 ]
then
  echo "test3 failed, exiting"
  exit
else
  echo "test3 successful"
fi
echo
echo "All tests successful"