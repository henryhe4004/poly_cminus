// int add2(int a, int b) { return a + b; }

// int add3(int a, int b, int c) { return a + b + c; }

int add10(int a, int b, int c, int d, int e, int f, int g, int h, int i, int j) {
    // int temp = add2(a, b);
    int ans = a + b + c + d + e + f + g + h + i + j;
    putint(a);
    putint(b);
    putint(c);
    putint(d);
    putint(e);
    putint(f);
    putint(g);
    putint(h);
    putint(i);
    putint(j);
    return ans;
}

int main() {
    int a = 1;
    int b = 2;
    int c = 3;
    int d = 4;
    int e = 5;
    int f = 6;
    int g = 7;
    int h = 8;
    int i = 9;
    int j = 10;
    int result = add10(a, b, c, d, e, f, g, h, i, j);
    return result;
}
 
