MODULE_NAME = iguard

BUILD_DIR = /lib/modules/$(shell uname -r)/build
HEADERS_EXIST = $(shell if [ -d "${BUILD_DIR}" ]; then echo "${BUILD_DIR}"; else echo ""; fi)

ifneq (${HEADERS_EXIST}, ${BUILD_DIR})
	$(warning "Check you have linux headers installed")
	$(error   "${BUILD_DIR} does not exist!")
endif

SRC := iguard-kmod.c iguard-utils.c

$(MODULE_NAME)-objs += $(SRC:.c=.o)

obj-m += $(MODULE_NAME).o


all: kmodule dummy

kmodule:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

dummy:
	gcc dummy.c -pthread -O0 -o dummy

benchmark_retired_instructions:
	unifdef -D BENCHMARK iguard-api.h > iguard-benchmark-api.h | true
	gcc benchmark_retired_instructions.c -pthread -Os -o benchmark_retired_instructions
	rm iguard-benchmark-api.h
	
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm dummy benchmark_retired_instructions | true
	
test:
	sudo insmod iguard.ko
	./dummy

	sudo rmmod iguard
	sudo insmod iguard.ko
	./dummy

	sudo rmmod iguard

	@echo "\n[=] You should see 2 incoming signals"
