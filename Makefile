C = gcc # compiler
CFLAGS = -Wall -g # Flag for implicit rules. Turn on debug info
LIB = -fopenmp # libraries
LIB_KLEIN = klein# file name
FIC = cipher# file name
FIC_ATTACK = attack# file name

cipher:
	$(C) $(FIC).c $(LIB_KLEIN).c -o $(FIC) $(CFLAGS) $(LIB)

attack:
	$(C) $(FIC_ATTACK).c $(LIB_KLEIN).c -o $(FIC_ATTACK) $(CFLAGS) $(LIB)

clean:
	rm -f $(FIC)
	rm -f $(FIC_ATTACK)
	rm -f $(LIB_KLEIN)
