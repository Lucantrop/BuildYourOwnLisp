#include <stdio.h>

typedef struct {
    float a;
    float b;
} point;

int add_together(int x, int y) {
    int result = x + y;
    return result;
}

void lewp(int n) {
    for(int i = 0; i < n; i++){
        puts("Hello, Function!");
    }
}

int main(int argc, char** argv) {
    int x1 = add_together(10, 18);
    if (x1 > 10 && x1 < 100) {
        puts("x is greater than ten and less than one hundred!");
    } else {
        puts("x is either less than eleven or greater than ninety-nine!");
    }
    int i = 5;
    while (i > 0) {
        puts("Hello, World!");
        i = i-1;
    }
    for (int i = 0; i < 5; i++) {
        puts("Hello, for-loop!");
    }
    lewp(10);

    return 0;
}