int main(){
    const int a[4] = {1, 2, 3, 4};
    int b[4]= {};
    int c[4]= {5, 6, 7, 8};
    int d[4] = {1, 3, 5, a[3]};
    int e[4]= {d[2], c[2], 3, 4};
    //e[3]=4 e[0]=d[2]=5 e[1]=7 d[0]=1
    return e[3] + e[0] + e[1] + d[0];
}