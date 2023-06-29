int main(void) {
    int i;
    int j;
    int k;
    int a;
    int b;
    int c;
    int sum;
    a = 10;
    b = 12;
    c = 8;
    sum = 10;
    i = 0;
    while (i < 2) {
        j = 0;
        while (j < 2) {
            k = 0;
            a=a+1;
            while(k<10){
                sum = sum+a+c+b;
                c = c+1;
                a = a+1;
                b = b+1;
                k = k+1;
            }
            j = j+1;
        }
        i = i + 1;
    }
    return sum;
}
