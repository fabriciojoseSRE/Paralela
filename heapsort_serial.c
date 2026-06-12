/*
 * heapsort_serial.c
 * Heap Sort - versao SERIAL
 *
 * Uso: ./heapsort_serial <tamanho> <vetor.in>
 * Saida: vetor_ordenado.out
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>

static void troca(float *a, float *b) { float t = *a; *a = *b; *b = t; }

/* Desce o elemento da posicao i ate restaurar a propriedade de max-heap,
 * considerando um heap de tamanho 'tam'. */
static void desce(float *v, int i, int tam) {
    for (;;) {
        int esq = 2 * i + 1, dir = 2 * i + 2, maior = i;
        if (esq < tam && v[esq] > v[maior]) maior = esq;
        if (dir < tam && v[dir] > v[maior]) maior = dir;
        if (maior == i) break;
        troca(&v[i], &v[maior]);
        i = maior;
    }
}

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

    /* Fase 1: construcao do max-heap (bottom-up) */
    for (int i = n / 2 - 1; i >= 0; i--)
        desce(v, i, n);

    /* Fase 2: extracao sucessiva do maximo */
    for (int fim = n - 1; fim > 0; fim--) {
        troca(&v[0], &v[fim]);   /* maior vai para o final */
        desce(v, 0, fim);        /* restaura o heap reduzido */
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
