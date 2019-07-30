make clean && make 
./ssd --gcsync --ndisk 4 --diskid 0 --gc_time_window 100000000 tracefile/AzureStorage8x.trace
./ssd --gcsync --ndisk 4 --diskid 0 --gc_time_window 200000000 tracefile/AzureStorage8x.trace
./ssd --gcsync --ndisk 4 --diskid 0 --gc_time_window 500000000 tracefile/AzureStorage8x.trace
./ssd --gcsync --ndisk 4 --diskid 0 --gc_time_window 1000000000 tracefile/AzureStorage8x.trace
./ssd --gcsync --ndisk 4 --diskid 0 --gc_time_window 5000000000 tracefile/AzureStorage8x.trace
./ssd --gcsync --ndisk 4 --diskid 0 --gc_time_window 10000000000 tracefile/AzureStorage8x.trace
