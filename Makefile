C = gcc # compiler
CFLAGS = -Wall -g # Flag for implicit rules. Turn on debug info
LIB = -fopenmp # libraries
FIC = klein# file name
FIC_ATTACK = attack# file name

run:
	$(C) $(FIC).c -o $(FIC) $(CFLAGS) $(LIB)

attack:
	$(C) $(FIC_ATTACK).c $(FIC).c -o $(FIC_ATTACK) $(CFLAGS) $(LIB)
clean:
	rm -f $(FIC)
	rm -f $(FIC_ATTACK)
