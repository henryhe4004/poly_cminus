int main(void){
    int a;
    int b;
    int c;
    int d;
    a =10;
    b =15;
    if(a<b){
        a =a-b;
        c = a;
    }else{
        a = a+b;
        c = a;
    }
    c = c+b;
    return c;
}