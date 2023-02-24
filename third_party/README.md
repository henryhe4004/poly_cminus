isl用的是autotools+makefile，要和我们现有的cmake结合起来有些麻烦，请手动解压并编译~

```
tar xf isl-isl-0.24.tar.gz
cd isl-isl-0.24
./autogen.sh
./configure --with-clang=system --prefix=${PWD}/../../build/isl
make
make install
```
可能需要安装libclang-dev和gmp
