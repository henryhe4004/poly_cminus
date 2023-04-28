int main(void) {
    int i;
    int j;
    int a;
    int b;
    int c;
    int sum;
    a = 10;
    b = 9;
    c = 8;
    i = 0;
    sum = 10;
    while (i < 2) {
        j = 0;
        while (j < 2) {
            sum = sum+a+b+c+i+j;
            a = a+1;
            b = b+1;
            c = c+1;
            j = j +1;
        }
        i = i + 1;
    }
    return sum;
}
