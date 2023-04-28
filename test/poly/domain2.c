int main(void) {
    int i;
    int j;
    int a;
    int b;
    int c;
    int d;
    int e;
    int f;
    int g;
    int h;
    int N;
    int sum;
    a = 10;
    b = 9;
    c = 8;
    d = 7;
    e = 6;
    f = 5;
    g = 4;
    h = 3;
    N = 2;
    i = 0;
    sum = 10;
    while (i < N) {
        j = 0;
        while (j < N) {
            sum = sum+a+b+c+d+e+f+g+h+N+i+j;
            a = a+1;
            b = b+1;
            c = c+1;
            d = d+1;
            e = e+1;
            f = f+1;
            g = g+1;
            h = h+1;
            j = j +1;
        }
        i = i + 1;
    }
    return sum;
}
