# Makefile - Trabalho de Programacao Paralela e Concorrente (OpenMP)
# Compila todos os programas (serial e paralelo) de cada algoritmo.
# Cada binario tem o mesmo nome do seu fonte.

CC      = gcc
CFLAGS  = -O2 -Wall -fopenmp
LDFLAGS = -lm

BINARIOS = matrixmult_serial   matrixmult_paralelo \
           selection_serial    selection_paralelo  \
           heapsort_serial     heapsort_paralelo   \
           histograma_serial   histograma_paralelo

all: $(BINARIOS)

matrixmult_serial:    matrixmult_serial.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

matrixmult_paralelo:  matrixmult_paralelo.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

selection_serial:     selection_serial.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

selection_paralelo:   selection_paralelo.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

heapsort_serial:      heapsort_serial.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

heapsort_paralelo:    heapsort_paralelo.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

histograma_serial:    histograma_serial.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

histograma_paralelo:  histograma_paralelo.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(BINARIOS) *.o *.out *.in

.PHONY: all clean
