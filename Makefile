C = gcc # compiler
CFLAGS = -Wall -g # Flag for implicit rules. Turn on debug info
LIB = -fopenmp # libraries
FIC_LIB = klein# file name
FIC = cipher# file name
FIC_ATTACK = attack# file name

cipher:
	$(C) $(FIC).c $(FIC_LIB).c -o $(FIC) $(CFLAGS) $(LIB)

attack:
	$(C) $(FIC_ATTACK).c $(FIC_LIB).c -o $(FIC_ATTACK) $(CFLAGS) $(LIB)

clean:
	rm -f $(FIC)
	rm -f $(FIC_ATTACK)
	rm -f $(FIC_LIB)
