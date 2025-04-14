#define ASTRON_HELIOS_IMPLEMENTATION
#include "../helios.h"
#define ASTRON_GE_USE_TOML
#include "../ge.h"

void EofError(void) {
    HeliosAllocator allocator = HeliosNewMallocAllocator();
    const char *buf = "world = 1\r\n\r\nhello = ";
    UZ buf_count = strlen(buf);
    char err_buf[512];
    UZ err_buf_count = sizeof(err_buf);
    GeTomlTable *table = GeTomlParseBuffer(allocator,
                                           buf,
                                           buf_count,
                                           err_buf,
                                           err_buf_count);

    HELIOS_VERIFY(table == NULL);

    HELIOS_VERIFY(strncmp(err_buf, "3:", 2) == 0);
    HELIOS_VERIFY(strncmp(err_buf + 2, "8:", 2) == 0);
    const char *eof_msg = " unexpected EOF";
    HELIOS_VERIFY(strncmp(err_buf + 4, eof_msg, strlen(eof_msg)) == 0);
}

void TokenMismatchError(void) {
    HeliosAllocator allocator = HeliosNewMallocAllocator();
    const char *buf = "\nhello = [}";
    UZ buf_count = strlen(buf);
    char err_buf[512];
    UZ err_buf_count = sizeof(err_buf);
    GeTomlTable *table = GeTomlParseBuffer(allocator,
                                           buf,
                                           buf_count,
                                           err_buf,
                                           err_buf_count);

    HELIOS_VERIFY(table == NULL);

    HELIOS_VERIFY(strncmp(err_buf, "2:", 2) == 0);
    HELIOS_VERIFY(strncmp(err_buf + 2, "10:", 2) == 0);
    const char *eof_msg = " expected an expression";
    HELIOS_VERIFY(strncmp(err_buf + 5, eof_msg, strlen(eof_msg)) == 0);
}

void Basic(void) {
    HeliosAllocator allocator = HeliosNewMallocAllocator();
    const char *buf = "[hello.table]\nworld = \"1\"\nis_good = true\n is_bad = false\n[hello.compound]\narr = [1, 2, 3,]\ntab = { key_a = 1, key_b = 2 }";
    UZ buf_count = strlen(buf);
    char err_buf[512];
    UZ err_buf_count = sizeof(err_buf);
    GeTomlTable *table = GeTomlParseBuffer(allocator,
                                           buf,
                                           buf_count,
                                           err_buf,
                                           err_buf_count);

    HELIOS_VERIFY(table != NULL);
}

int main(void) {
    EofError();
    TokenMismatchError();
    Basic();
    return 0;
}
