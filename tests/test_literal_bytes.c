#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    static const unsigned char expected[] = {0xbe, 0xc8, 0xb3, 0xe7, 'A', '7', 0};
    static const char text[] = "안녕A7";
    assert(sizeof text == sizeof expected);
    assert(!memcmp(text, expected, sizeof text));
    puts("literal bytes: CP949 matches the Win32 execution charset");
    return 0;
}
