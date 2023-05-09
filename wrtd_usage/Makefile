EXEC=$(patsubst %.c,%,$(wildcard *.c))

CC=$(CROSS_COMPILE)gcc

LDLIBS=-lwrtd -ladc

CFLAGS= -std=c99 \
	-I$(BUILD_DIR)/wrtd/software/include \
	-I$(BUILD_DIR)/wrtd/software/lib \
	-I$(BUILD_DIR)/adc-lib/lib \
	-I$(BUILD_DIR)/fmc-adc-100m14b4cha/software/kernel

all: $(EXEC)

clean:
	rm $(EXEC)
