int main(void) {
    int i;
    int j;
    int a;
    int b;
    int sum;
    a = 10;
    sum = 10;
    i = 0;
    b = 0;
    while (i < 2) {
        sum =sum + a+b;
        a = sum+1;
        b = b+1;
        i = i + 1;
    }
    return sum;
}
