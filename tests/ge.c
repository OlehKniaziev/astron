#define ASTRON_HELIOS_IMPLEMENTATION
#include "../helios.h"
#define ASTRON_GE_USE_TOML
#include "../ge.h"

int main(void) {
    HeliosAllocator allocator = HeliosNewMallocAllocator();
    const char *buf = "[hello]\nworld = \"1\"\nis_good = true\n is_bad = false\narr = [1, 2, 3,]\ntab = { key_a = 1, key_b = 2 }";
    UZ buf_count = strlen(buf);
    char err_buf[512];
    UZ err_buf_count = sizeof(err_buf);
    GeTomlTable table;

    B32 success = GeTomlParseBuffer(allocator,
                                    buf,
                                    buf_count,
                                    err_buf,
                                    err_buf_count,
                                    &table);

    HELIOS_VERIFY(success);

    return 0;
}
