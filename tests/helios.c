#define ASTRON_HELIOS_IMPLEMENTATION
#include "../helios.h"

void ReadFileSuccess(void) {
    HeliosAllocator allocator = HeliosNewMallocAllocator();
    HeliosStringView file_contents = HeliosReadEntireFile(allocator, HELIOS_SV_LIT(__FILE__));
    HELIOS_VERIFY(file_contents.data != NULL);

    HELIOS_VERIFY(HeliosStringViewStartsWith(file_contents, "#define ASTRON_HELIOS_IMPLEMENTATION"));
}

void FormatAppendCorrect(void) {
    HeliosAllocator alloc = HeliosNewMallocAllocator();
    HeliosString8 s = {.allocator = alloc};

    HeliosString8FormatAppend(&s, "hello %d", 1);
    HELIOS_VERIFY(strcmp((char *)s.data, "hello 1") == 0);
    HeliosString8FormatAppend(&s, " world %s", "yes");
    HELIOS_VERIFY(strcmp((char *)s.data, "hello 1 world yes") == 0);
}

int main() {
    ReadFileSuccess();
    FormatAppendCorrect();
}
