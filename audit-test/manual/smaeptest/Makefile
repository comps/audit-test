obj-m += smaeptest.o

all: user
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

user: user.c

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f ./modules.order
	rm -f user
