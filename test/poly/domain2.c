int main() {
    int i, j, k;
    const int N = 10, M = 10, K = 20;
    int A[N], B[N], Z[2 * N];
    // int X[N][M], C[N][K], D[K][M];
    i = 0;
    while (i < N) {
        j = 0;
        while (j < N) {
            /* foo:  */ Z[i + j] = Z[i + j] + A[i] * B[j];
            j = j + 1;
        }
        i = i + 1;
    }
}