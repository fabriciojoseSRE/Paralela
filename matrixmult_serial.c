/*
 * matrixmult_serial.c
 * Multiplicacao de matrizes quadradas (NxN) - versao SERIAL
 *
 * Uso: ./matrixmult_serial <ordem> <matriz1.in> <matriz2.in>
 * Saida: matriz_mult.out
 *
 * Se os arquivos de entrada nao existirem, o programa gera matrizes
 * aleatorias e as salva, para que serial e paralelo usem os MESMOS dados.
 *
 * Os dados sao numeros do tipo float.
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>

/* Le todos os N*N floats de um arquivo para o vetor m (matriz linearizada) */
static void carrega_matriz(const char *nome, float *m, int n) {
    FILE *f = fopen(nome, "r");
    if (!f) { perror("fopen (carregar)"); exit(EXIT_FAILURE); }
    for (long i = 0; i < (long)n * n; i++) {
        if (fscanf(f, "%f", &m[i]) != 1) {
            fprintf(stderr, "Erro lendo o arquivo %s\n", nome);
            exit(EXIT_FAILURE);
        }
    }
    fclose(f);
}

/* Gera uma matriz NxN aleatoria e a salva em arquivo */
static void gera_matriz(const char *nome, int n, unsigned int semente) {
    FILE *f = fopen(nome, "w");
    if (!f) { perror("fopen (gerar)"); exit(EXIT_FAILURE); }
    srand(semente);
    for (long i = 0; i < (long)n * n; i++)
        fprintf(f, "%f\n", (float)(rand() % 10000) / 100.0f);
    fclose(f);
}

static int arquivo_existe(const char *nome) {
    FILE *f = fopen(nome, "r");
    if (f) { fclose(f); return 1; }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <ordem> <matriz1.in> <matriz2.in>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int n = atoi(argv[1]);
    const char *arq1 = argv[2];
    const char *arq2 = argv[3];

    /* Se as entradas nao existem, cria dados aleatorios reproduziveis */
    if (!arquivo_existe(arq1)) gera_matriz(arq1, n, 1);
    if (!arquivo_existe(arq2)) gera_matriz(arq2, n, 2);

    float *A = malloc((size_t)n * n * sizeof(float));
    float *B = malloc((size_t)n * n * sizeof(float));
    float *C = malloc((size_t)n * n * sizeof(float));
    if (!A || !B || !C) { fprintf(stderr, "Sem memoria\n"); return EXIT_FAILURE; }

    carrega_matriz(arq1, A, n);
    carrega_matriz(arq2, B, n);

    /* ---- Medicao APENAS do algoritmo ---- */
    double t0 = omp_get_wtime();

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            float soma = 0.0f;
            for (int k = 0; k < n; k++)
                soma += A[i * n + k] * B[k * n + j];
            C[i * n + j] = soma;
        }
    }

    double t1 = omp_get_wtime();
    /* -------------------------------------- */

    FILE *out = fopen("matriz_mult.out", "w");
    if (!out) { perror("fopen (saida)"); return EXIT_FAILURE; }
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++)
            fprintf(out, "%f ", C[i * n + j]);
        fprintf(out, "\n");
    }
    fclose(out);

    fprintf(stderr, "[serial]  ordem=%d  tempo=%.6f s\n", n, t1 - t0);

    free(A); free(B); free(C);
    return EXIT_SUCCESS;
}
