float add(float a, float b) { return a + b; }

float sub(float a, float b) { return a - b; }

float add_sub(float a, float b, float c) { return add(a, sub(b, c)); }

int main() {
    float a = 111;
    float b = 22;
    float c = add(a, b);
    putfloat(c);
    float d = sub(a, b);
    putfloat(d);
    float e = add_sub(a, b, d);
    putfloat(e);
}
