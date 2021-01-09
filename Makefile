OUTPUTS = Server Client

all: $(OUTPUTS)

clean:
	rm -f $(OUTPUTS)
	rm -f *.o

.PHONY: all clean

%: %.c
	$(CC) -lpthread -lncurses -o $@ $<