MPICC=mpicc

all : greq

egreq : egreq.c
	$(MPICC) -o $@ $^

run : egreq
	mpirun -np 2 ./egreq

clean:
	rm -fr egreq egreqmpc 

#MPC Specific
MPCCC=mpc_cc

egreqmpc : egreq.c
	$(MPCCC) -DMPC -o $@ $^

runmpc : egreqmpc
	mpcrun -n=2 ./egreqmpc
