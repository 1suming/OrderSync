rm -f ../log/*
ulimit -c unlimited
export LD_LIBRARY_PATH=../lib/$LD_LIBRARY_PATH
./OrderSyncServer -p 6700 -s 1 -h 0.0.0.0 -d
#./PayGate ../conf/config.xml -p 4701 -s 2

