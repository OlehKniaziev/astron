#define ASTRON_HELIOS_IMPLEMENTATION
#include "../helios.h"

void ReadFileSuccess(void) {
    HeliosAllocator allocator = HeliosNewMallocAllocator();
    HeliosStringView file_contents = HeliosReadEntireFile(allocator, HELIOS_SV_LIT(__FILE__));
    HELIOS_VERIFY(file_contents.data != NULL);

    HELIOS_VERIFY(HeliosStringViewStartsWith(file_contents, "#define ASTRON_HELIOS_IMPLEMENTATION"));
}

int main() {
    ReadFileSuccess();
}
