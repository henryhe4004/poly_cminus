int main(void) {
    int i;
    int j;
    int k;
    int A[20];
    int B[20];
    int Z[40];
    i = 0;
    while (i < 20) {
        j = 0;
        while (j < 20) {
            /* foo:  */ Z[i + j] = Z[i + j] + A[i] * B[j];
            j = j + 1;
        }
        i = i + 1;
    }
}
/*
第一步要先构造iteration domain，然后是schedule，access relation，这样应该就得到了schedule tree?
参考http://playground.pollylabs.org/
三个statement(foo, S, T)的iteration domain分别是
{ foo[i,j] : 0 <= i < 10 and 0 <= j < 10 }
{ S[i, j] : 0 <= i < 10 and 0 <= j < 10 }
{ T[i, j, k] : 0 <= i < 10 and 0 <= j < 10 and 0 <= k < 20 }
这些字符串要传给 isl::union_set，例如，
isl::union_set A(ctx, "{ A[i] : 0 <= i < 10 }");
(ctx大概是isl的管理模块吧，详见Poly/test.cc)

我觉得比较麻烦的地方是llvm ir会引入很多的临时变量，要从一大堆的虚拟寄存器中找到对应源代码的statement（例如T）
初步想法是从store（数组存储）语句往上找，或者从非归纳变量的phi指令（例如一维数组求和）来构造原有ast的迭代域

另外关于归纳变量ijk等，我们可以先只处理 i++ 这种最简单的，别的情况好像没怎么出现
*/
