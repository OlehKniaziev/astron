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

void Nested(void) {
    HeliosAllocator allocator = HeliosNewMallocAllocator();
    const char *buf = "value = [{a.b = 1}]";
    UZ buf_count = strlen(buf);
    char err_buf[512];
    UZ err_buf_count = sizeof(err_buf);
    GeTomlTable *table = GeTomlParseBuffer(allocator,
                                           buf,
                                           buf_count,
                                           err_buf,
                                           err_buf_count);

    HELIOS_VERIFY(table != NULL);

    GeTomlValue *value = GeTomlTableFind(table, "value");
    HELIOS_VERIFY(value != NULL);
    HELIOS_VERIFY(value->type == GeTomlValueType_Array);

    GeTomlArray array = value->a;
    HELIOS_VERIFY(array.count = 1);

    GeTomlValue subvalue = GeTomlArrayAt(&array, 0);
    HELIOS_VERIFY(subvalue.type == GeTomlValueType_Table);

    GeTomlTable *subtable = subvalue.t;
    HELIOS_VERIFY(HeliosStringViewEqualCStr(subtable->key, "a"));
    HELIOS_VERIFY(subtable->value.type == GeTomlValueType_Table);

    GeTomlTable *subsubtable = subtable->value.t;
    HELIOS_VERIFY(HeliosStringViewEqualCStr(subsubtable->key, "b"));
    HELIOS_VERIFY(subsubtable->value.type == GeTomlValueType_Int);
    HELIOS_VERIFY(subsubtable->value.i == 1);
}

void Integers(void) {
    HeliosAllocator allocator = HeliosNewMallocAllocator();
    const char *buf = "two = 0b11\neight = 0o777\nten = 2005\nsixteen = 0xaBcD\n";
    UZ buf_count = strlen(buf);
    char err_buf[512];
    UZ err_buf_count = sizeof(err_buf);
    GeTomlTable *table = GeTomlParseBuffer(allocator,
                                           buf,
                                           buf_count,
                                           err_buf,
                                           err_buf_count);

    HELIOS_VERIFY(table != NULL);

    HELIOS_VERIFY(HeliosStringViewEqualCStr(table->key, "two"));
    HELIOS_VERIFY(table->value.type == GeTomlValueType_Int);
    HELIOS_VERIFY(table->value.i == 3);

    HELIOS_VERIFY(HeliosStringViewEqualCStr(table->next->key, "eight"));
    HELIOS_VERIFY(table->next->value.type == GeTomlValueType_Int);
    HELIOS_VERIFY(table->next->value.i == 0777);

    HELIOS_VERIFY(HeliosStringViewEqualCStr(table->next->next->key, "ten"));
    HELIOS_VERIFY(table->next->next->value.type == GeTomlValueType_Int);
    HELIOS_VERIFY(table->next->next->value.i == 2005);

    HELIOS_VERIFY(HeliosStringViewEqualCStr(table->next->next->next->key, "sixteen"));
    HELIOS_VERIFY(table->next->next->next->value.type == GeTomlValueType_Int);
    HELIOS_VERIFY(table->next->next->next->value.i == 0xABCD);
}

int main(void) {
    EofError();
    TokenMismatchError();
    Basic();
    Nested();
    Integers();
    return 0;
}
