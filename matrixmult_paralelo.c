/*
 * matrixmult_paralelo.c
 * Multiplicacao de matrizes quadradas (NxN) - versao PARALELA (OpenMP)
 *
 * Uso: ./matrixmult_paralelo <ordem> <matriz1.in> <matriz2.in>
 * Saida: matriz_mult.out
 *
 * Numero de threads controlado pela variavel de ambiente OMP_NUM_THREADS.
 * Ex.: OMP_NUM_THREADS=4 ./matrixmult_paralelo 1000 m1.in m2.in
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>

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

    /*
     * Cada elemento C[i][j] e' escrito por um unico (i,j): NAO ha regiao critica
     * nem dependencia de dados entre iteracoes do laco externo. Por isso podemos
     * paralelizar o laco externo livremente. 'collapse(2)' funde os dois lacos
     * externos em um unico espaco de iteracoes maior, melhorando o balanceamento.
     * As variaveis i, j, k e soma sao privadas de cada thread (i e j por serem
     * indices dos lacos paralelizados; k e soma por estarem declaradas dentro).
     */
    #pragma omp parallel for collapse(2) schedule(static)
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

    fprintf(stderr, "[paralelo] ordem=%d  threads=%d  tempo=%.6f s\n",
            n, omp_get_max_threads(), t1 - t0);

    free(A); free(B); free(C);
    return EXIT_SUCCESS;
}
