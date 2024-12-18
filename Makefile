C = gcc # compiler
CFLAGS = -Wall # Flag for implicit rules. Turn on debug info
LIB = -fopenmp # libraries
FIC = klein # file name

run:
	$(C) $(FIC).c -o $(FIC) $(CFLAGS) $(LIB)

clean:
	rm -f $(FIC)
