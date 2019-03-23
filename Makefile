# ssdsim linux support
all:ssd 
new: ssd-test

clean:
	rm -f ssd *.o *~
.PHONY: clean

ssd-test: test.o avlTree.o flash.o initialize.o pagemap.o
	cc -g -o ssd test.o avlTree.o flash.o initialize.o pagemap.o
test.o: flash.h initialize.h pagemap.h
	gcc -c -g test.c
ssd: ssd.o avlTree.o flash.o initialize.o pagemap.o raid.o
	cc -g -o ssd ssd.o avlTree.o flash.o initialize.o pagemap.o raid.o
#	rm *.o
ssd.o: flash.h initialize.h pagemap.h raid.h
	gcc -c -g ssd.c
flash.o: pagemap.h
	gcc -c -g flash.c
initialize.o: avlTree.h pagemap.h
	gcc -c -g initialize.c
pagemap.o: initialize.h
	gcc -c -g pagemap.c
avlTree.o: 
	gcc -c -g avlTree.c
raid.o:
	gcc -c -g raid.c

