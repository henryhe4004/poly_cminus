// #include<iostream>
// using namespace std;
int main() {
    int i, j, k;
    const int N = 3;
    int A[N], B[N], Z[2 * N];
    k=0;
    while(k<6){
        Z[k]=0;
        k=k+1;
    }
    k=0;
    while(k<N){
        A[k]=1+k;
        k=k+1;
    }
    k=0;
    while(k<N){
        B[k]=1+k;
        k=k+1;
    }
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
    // cout<<Z[3];
    return Z[3];
}
