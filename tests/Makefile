CC=g++
LDFLAGS=-L../lib/counters -lcounters -fopenmp

all: LCALS_energy_calc LCALS_pressure_calc LCALS_vol3d_calc

LCALS_vol3d_calc:
	$(CC) LCALS_vol3d_calc.cpp -o LCALS_vol3d_calc $(LDFLAGS)

LCALS_energy_calc: 
	$(CC) LCALS_energy_calc.cpp -o LCALS_energy_calc $(LDFLAGS)

LCALS_pressure_calc:
	$(CC) LCALS_pressure_calc.cpp -o LCALS_pressure_calc $(LDFLAGS)

clean:
	rm -f LCALS_energy_calc LCALS_pressure_calc LCALS_vol3d_calc

# gcc ./LCALS_pressure_calc.c -o LCALS_pressure_calc -L/home/hpc1/installs/data_collection/counters/ -lcounters