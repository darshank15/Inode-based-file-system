CC = g++
CFLAGS = -Wall -std=c++14 -g
DEPS = inode.h
OBJ = file_read_write.o file_operation.o inode.o

%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

ibfs: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

.PHONY: clean

clean:
	rm -rf *.o ibfs