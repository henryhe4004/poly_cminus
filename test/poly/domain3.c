//#include <iostream>
//using namespace std;
int main(){
    const int N = 50;
    int Z[N][N];
    int i, j, k,m,n,p,q;
    i = 0;
    j = 0;
    while(i<N){
        j=0;
        while(j<N){
            Z[i][j]=1;
            j = j + 1;
        }
        i = i + 1;
    }
    m = 0;
    while(m<N){
        k= 0;
        while(k<N){
            i = 1;
            while(i<N-1){
                j = 1;
                while(j<N-1){
                    Z[i+1][j] = Z[i][j+1]-Z[i][j]+Z[i][j-1];
                    j = j + 1;
                }
                i = i + 1;
            }
            k = k + 1;
        }
        m= m + 1;
    }
//    cout<<Z[5][5];
    return 0;
}