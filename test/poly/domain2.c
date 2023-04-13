// #include<iostream>
// using namespace std;
int main() {
    int i, j, k,m,n,p,q;
    const int N = 50;

    int A[N], B[N], Z[2 * N];
    k=0;
    while(k<2*N){
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
    q = 0;
    while(q<N){
        p = 0;
        while(p<N){
            n = 0;
            while(n<N){
                m = 0;
                while(m<N){
                    i = 0;
                    while (i < N) {
                        j = 0;
                        while (j < N) {
                            /* foo:  */ Z[i + j] = Z[i + j] + A[i] * B[j];
                            j = j + 1;
                        }
                        i = i + 1;
                    
                    }
                    m = m + 1;
                }
                n = n + 1;
            }
            p = p + 1;
        }
        q = q+1;
    }
 
    return Z[70];
}
