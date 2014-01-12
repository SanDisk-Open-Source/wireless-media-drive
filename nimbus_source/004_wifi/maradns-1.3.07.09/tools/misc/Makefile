ROBJECTS=../../rng/rng-alg-fst.o ../../rng/rng-api-fst.o
EXECS=marahash mqhash

FLAGS = -g
#FLAGS = -g

all: $(EXECS)

clean:
	rm -f core $(EXECS)

marahash: $(ROBJECTS) rnghash.c
	$(CC) $(FLAGS) -o marahash rnghash.c $(ROBJECTS) 

mqhash: $(ROBJECTS) mqhash.c
	$(CC) $(FLAGS) -o mqhash mqhash.c $(ROBJECTS) 

