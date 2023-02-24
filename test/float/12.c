int add2(int a, int b) { return a + b; }

int add3(int a, int b, int c) { return a + b + c; }

int add5(int a, int b, int c, int d, int e) {
    int temp2 = add2(a, b);
    // int temp3 = add2(c, d);
    int temp3 = add3(c, d, e);
    int ans = a + b + c + d + e;
    putint(a);
    putint(b);
    putint(c);
    putint(d);
    putint(e);
    putint(temp2);
    putint(temp3);
    return ans;
}

int main() {
    int a = 1;
    int b = 2;
    int c = 3;
    int d = 4;
    int e = 5;
    int result = add5(a, b, c, d, e);
    putint(result);
    return 0;
}
