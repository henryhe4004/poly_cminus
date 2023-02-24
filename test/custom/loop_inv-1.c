// licm中将icmp/fcmp结果外提后需要保存
// 不能在gen_br中调用gen_cmp
int main() {
    int a = getint();
    int b = getint();
    int i = 0;
    while (i < 10) {
        if (a < b)
            putint(10);
        else
            putint(20);
        i = i + 1;
    }
}
