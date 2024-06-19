
obj-m = work_queue.o

all:
	make -C /lib/modules/6.1.10-akshu/build M=`pwd` modules

clean:
	make -C /lib/modules/6.1.10-akshu/build M=`pwd` clean

.PHONY: all clean

