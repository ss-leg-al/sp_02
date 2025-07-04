
TARGET = my_xdp


CLANG = clang
CFLAGS = -O2 -g -Wall -target bpf -D__TARGET_ARCH_arm64 -I./headers


BPF_OBJ = $(TARGET).o

all: $(BPF_OBJ)

$(BPF_OBJ): $(TARGET).c
	$(CLANG) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(BPF_OBJ)