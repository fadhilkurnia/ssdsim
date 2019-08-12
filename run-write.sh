./ssd tracefile/write2.trace
./ssd --gcsync --ndisk 4 --diskid 0 --gc_time_window 10000000 tracefile/write2.trace
./ssd --gcsync --ndisk 4 --diskid 0 --gc_time_window 100000000 tracefile/write2.trace
./ssd --gcsync --ndisk 4 --diskid 0 --gc_time_window 10000000000 tracefile/write2.trace
