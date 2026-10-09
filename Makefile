ifneq ($(KERNELRELEASE),)

obj-m += vtfs.o 

src_files := $(wildcard $(src)/source/*.c)
vtfs-y := $(patsubst $(src)/%.c,%.o,$(src_files))

ccflags-y += -Wall -g

$(info Sources: $(src_files))
$(info Objects: $(vtfs-y))

else

PWD := $(CURDIR) 
KDIR = /lib/modules/`uname -r`/build

.PHONY: all clean deploy

all:
	make -C $(KDIR) M=$(PWD) modules 

clean:
	make -C $(KDIR) M=$(PWD) clean
	rm -rf .cache

deploy:
	rsync -av --delete -e "ssh -p 2222" ./ test@localhost:~/vtfs/
	ssh -p 2222 test@localhost "cd ~/vtfs && make clean && make"

endif
