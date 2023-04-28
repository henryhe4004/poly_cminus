int main(void){
    int a;
    int b;
    int c;
    int k;
    b = 10;
    c = 20;
    k = 0;
    while(k<1){
        if(b<c){
            a=b+c;
        }else{
            a=c-b;
        }
        a = a+1;
        k = k+1;
    }
    return a;
}