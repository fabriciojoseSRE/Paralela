/*
 * selection_serial.c
 * Selection Sort - versao SERIAL
 *
 * Uso: ./selection_serial <tamanho> <vetor.in>
 * Saida: vetor_ordenado.out
 *
 * Se o arquivo de entrada nao existir, gera um vetor aleatorio de floats.
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>

static int arquivo_existe(const char *nome) {
    FILE *f = fopen(nome, "r");
    if (f) { fclose(f); return 1; }
    return 0;
}

static void gera_vetor(const char *nome, int n) {
    FILE *f = fopen(nome, "w");
    if (!f) { perror("fopen (gerar)"); exit(EXIT_FAILURE); }
    srand(42);
    for (int i = 0; i < n; i++)
        fprintf(f, "%f\n", (float)(rand() % 1000000) / 100.0f);
    fclose(f);
}

static void carrega_vetor(const char *nome, float *v, int n) {
    FILE *f = fopen(nome, "r");
    if (!f) { perror("fopen (carregar)"); exit(EXIT_FAILURE); }
    for (int i = 0; i < n; i++)
        if (fscanf(f, "%f", &v[i]) != 1) {
            fprintf(stderr, "Erro lendo %s\n", nome); exit(EXIT_FAILURE);
        }
    fclose(f);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <tamanho> <vetor.in>\n", argv[0]);
        return EXIT_FAILURE;
    }
    int n = atoi(argv[1]);
    const char *arq = argv[2];

    if (!arquivo_existe(arq)) gera_vetor(arq, n);

    float *v = malloc((size_t)n * sizeof(float));
    if (!v) { fprintf(stderr, "Sem memoria\n"); return EXIT_FAILURE; }
    carrega_vetor(arq, v, n);

    /* ---- Medicao APENAS do algoritmo ---- */
    double t0 = omp_get_wtime();

    for (int i = 0; i < n - 1; i++) {
        int idx_min = i;
        for (int j = i + 1; j < n; j++) {
            if (v[j] < v[idx_min])
                idx_min = j;
        }
        if (idx_min != i) {
            float tmp = v[i];
            v[i] = v[idx_min];
            v[idx_min] = tmp;
        }
    }

    double t1 = omp_get_wtime();
    /* -------------------------------------- */

    FILE *out = fopen("vetor_ordenado.out", "w");
    if (!out) { perror("fopen (saida)"); return EXIT_FAILURE; }
    for (int i = 0; i < n; i++) fprintf(out, "%f\n", v[i]);
    fclose(out);

    fprintf(stderr, "[serial]  n=%d  tempo=%.6f s\n", n, t1 - t0);

    free(v);
    return EXIT_SUCCESS;
}
