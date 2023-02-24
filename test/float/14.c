int dfs(int a, int b, int c, int d, int e, int last) {
    if (b == 0)
        return 1;
    int ans = 0;
    putint(b);
    ans = dfs(a + 1, b - 1, c, d, e, 2);
    putint(b);
    return ans;
}

int main() {
    dfs(5, 5, 5, 5, 5, 5);
    return 0;
}
