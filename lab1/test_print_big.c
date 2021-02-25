#include <stdio.h>

#define page_size (1 << 12)

char buff[page_size * 2];
int main() {
    FILE* f = fopen("/dev/var1", "w");
    if (!f) {
        printf("File not found\n");
        return 0;
    }
    fwrite(buff, 1,  sizeof(buff), f);
    fclose(f);
    return 0;
}

