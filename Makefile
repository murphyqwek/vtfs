obj-m += source/vtfs.o 

PWD := $(CURDIR) 
KDIR = /lib/modules/`uname -r`/build
EXTRA_CFLAGS = -Wall -g

all:
	make -C $(KDIR) M=$(PWD) modules 

clean:
	make -C $(KDIR) M=$(PWD) clean
	rm -rf .cache

deploy:
	rsync -av --delete -e "ssh -p 2222" ./ test@localhost:~/vtfs/
	ssh -p 2222 test@localhost "cd ~/vtfs && make clean && make"
