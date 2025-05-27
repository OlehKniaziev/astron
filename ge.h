// TODOs:
// toml:
//   add spec conformance tests

#ifndef ASTRON_GE_H
#define ASTRON_GE_H

#ifndef ASTRON_HELIOS_H
#    error "'ge.h' requires 'helios.h' to be included first"
#endif // ASTRON_HELIOS_H

#ifdef ASTRON_HELIOS_IMPLEMENTATION
#    define ASTRON_GE_IMPLEMENTATION
#endif // ASTRON_HELIOS_IMPLEMENTATION

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// common definitions

typedef struct GeSourceLocation {
    U32 line;
    U32 column;
} GeSourceLocation;

HELIOS_INLINE B32 GeIsCharDigitInBase(HeliosChar c, U8 base) {
    if (base == 10) return c >= '0' && c <= '9';
    if (base == 16) return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    if (base == 8)  return c >= '0' && c <= '7';
    if (base == 2)  return c == '0' || c == '1';
    return 0;
}

#ifdef ASTRON_GE_USE_TOML

typedef U8 GeTomlValueType;
enum {
    GeTomlValueType_Int,
    GeTomlValueType_String,
    GeTomlValueType_Float,
    GeTomlValueType_Bool,
    GeTomlValueType_DateTime,
    GeTomlValueType_Array,
    GeTomlValueType_Table,

    GeTomlValueType_Max,
};

struct GeTomlValue;
typedef struct GeTomlValue GeTomlValue;
#ifdef ASTRON_ERMIS_H
    ERMIS_DECL_ARRAY(GeTomlValue, GeTomlArray)
#else
typedef struct GeTomlArray {
    GeTomlValue *items;
    UZ count;
    UZ capacity;
    HeliosAllocator allocator;
} GeTomlArray;
#endif // ASTRON_ERMIS_H
typedef struct GeTomlTable GeTomlTable;

struct GeTomlValue {
    GeTomlValueType type;

    union {
        S64 i;
        F64 f;
        B32 b;
        HeliosStringView s;
        GeTomlArray a;
        GeTomlTable *t;
    };
};

struct GeTomlTable {
    struct GeTomlTable *next;
    HeliosStringView key;
    GeTomlValue value;
};

HELIOS_DEF GeTomlTable *GeTomlParseBuffer(HeliosAllocator allocator,
                                          const char *buf,
                                          UZ buf_count,
                                          char *err_buf,
                                          UZ err_buf_count);

HELIOS_DEF GeTomlValue *GeTomlTableFind(GeTomlTable *table, const char *key);
HELIOS_DEF GeTomlValue *GeTomlTableFindSV(GeTomlTable *table, HeliosStringView sv);

HELIOS_DEF B32 GeTomlTableHas(GeTomlTable *table, const char *key);
HELIOS_DEF B32 GeTomlTableHasSV(GeTomlTable *table, HeliosStringView sv);
#endif // ASTRON_GE_USE_TOML

#ifdef __cplusplus
    }
#endif // __cplusplus

#ifdef ASTRON_GE_IMPLEMENTATION

HELIOS_INTERNAL GeSourceLocation _GeSourceLocationFromStream(HeliosString8Stream *stream) {
    UZ byte_offset = stream->byte_offset;
    UZ data_count = stream->count;
    const U8 *data = stream->data;
    // Reset the stream.
    HeliosString8StreamInit(stream, data, data_count);

    U32 col = 0;
    U32 line = 1;

    HeliosChar cur_char;
    while (stream->byte_offset != byte_offset) {
        HELIOS_ASSERT(HeliosString8StreamNext(stream, &cur_char));

        if (cur_char == '\n' || (cur_char == '\r' &&
                                 HeliosString8StreamNext(stream, &cur_char) &&
                                 cur_char == '\n')) {
            col = 0;
            ++line;
        } else {
            ++col;
        }
    }

    return (GeSourceLocation) {
        .column = col,
        .line = line,
    };
}

HELIOS_INTERNAL GeSourceLocation _GeSourceLocationFromTokenData(HeliosString8Stream *stream, HeliosStringView data) {
    UZ stream_data_count = stream->count;
    const U8 *stream_data = stream->data;
    // Reset the stream.
    HeliosString8StreamInit(stream, stream_data, stream_data_count);

    U32 col = 0;
    U32 line = 1;

    HeliosChar cur_char;
    do {
        HELIOS_ASSERT(HeliosString8StreamNext(stream, &cur_char));

        if (cur_char == '\n' || (cur_char == '\r' &&
                                 HeliosString8StreamNext(stream, &cur_char) &&
                                 cur_char == '\n')) {
            col = 0;
            ++line;
        } else {
            ++col;
        }
    } while (stream->data + stream->byte_offset != data.data);

    return (GeSourceLocation) {
        .column = col,
        .line = line,
    };
}

#ifdef ASTRON_GE_USE_TOML

typedef struct GeTomlParsingContext {
    HeliosString8Stream stream;
    char *err_buf;
    UZ err_buf_count;
    HeliosAllocator allocator;
} GeTomlParsingContext;

HELIOS_DEF GeTomlValue *GeTomlTableFindSV(GeTomlTable *table, HeliosStringView key) {
    for (; table != NULL; table = table->next) {
        if (HeliosStringViewEqual(table->key, key)) return &table->value;
    }

    return NULL;
}

HELIOS_DEF GeTomlValue *GeTomlTableFind(GeTomlTable *table, const char *key) {
    UZ sv_count = strlen(key);
    HeliosStringView sv = { .data = (const U8 *)key, .count = sv_count };
    return GeTomlTableFindSV(table, sv);
}

HELIOS_DEF B32 GeTomlTableHasSV(GeTomlTable *table, HeliosStringView key) {
    for (; table != NULL; table = table->next) {
        if (HeliosStringViewEqual(table->key, key)) return 1;
    }

    return 0;
}

HELIOS_DEF B32 GeTomlTableHas(GeTomlTable *table, const char *key) {
    UZ sv_count = strlen(key);
    HeliosStringView sv = { .data = (const U8 *)key, .count = sv_count };
    return GeTomlTableHasSV(table, sv);
}

typedef enum {
    GeTomlTokenType_LeftBracket,
    GeTomlTokenType_RightBracket,
    GeTomlTokenType_LeftBrace,
    GeTomlTokenType_RightBrace,
    GeTomlTokenType_Equals,
    GeTomlTokenType_Identifier,
    GeTomlTokenType_String,
    GeTomlTokenType_Int,
    GeTomlTokenType_Float,
    GeTomlTokenType_Comma,
    GeTomlTokenType_Dot,
    GeTomlTokenType_Newline,

    GeTomlTokenType_Illegal,
    GeTomlTokenType_UnterminatedString,
} GeTomlTokenType;

typedef struct GeTomlToken {
    GeTomlTokenType type;
    HeliosStringView value;
} GeTomlToken;

HELIOS_INTERNAL HELIOS_INLINE B32 _GeTomlIsValidIdentChar(HeliosChar c) {
    return c == '-' || c == '_' || HeliosCharIsAlnum(c);
}

HELIOS_INTERNAL HELIOS_INLINE B32 _GeTomlIsCharWhitespace(HeliosChar c) {
    return c == '\t' || c == ' ';
}

HELIOS_INTERNAL B32 _GeTomlNextToken(HeliosString8Stream *s, GeTomlToken *token) {
    HeliosChar cur_char;
    while (1) {
        if (!HeliosString8StreamNext(s, &cur_char)) return 0;
        if (!_GeTomlIsCharWhitespace(cur_char)) break;
    }

    switch (cur_char) {
    case '[': {
        token->type = GeTomlTokenType_LeftBracket;
        token->value = (HeliosStringView) { .data = s->data + s->byte_offset, .count = 1 };
        return 1;
    }
    case ']': {
        token->type = GeTomlTokenType_RightBracket;
        token->value = (HeliosStringView) { .data = s->data + s->byte_offset, .count = 1 };
        return 1;
    }
    case '{': {
        token->type = GeTomlTokenType_LeftBrace;
        token->value = (HeliosStringView) { .data = s->data + s->byte_offset, .count = 1 };
        return 1;
    }
    case '}': {
        token->type = GeTomlTokenType_RightBrace;
        token->value = (HeliosStringView) { .data = s->data + s->byte_offset, .count = 1 };
        return 1;
    }
    case '=': {
        token->type = GeTomlTokenType_Equals;
        token->value = (HeliosStringView) { .data = s->data + s->byte_offset, .count = 1 };
        return 1;
    }
    case '\n': {
        token->type = GeTomlTokenType_Newline;
        token->value = (HeliosStringView) { .data = s->data + s->byte_offset, .count = 1 };
        return 1;
    }
    case ',': {
        token->type = GeTomlTokenType_Comma;
        token->value = (HeliosStringView) { .data = s->data + s->byte_offset, .count = 1 };
        return 1;
    }
    case '.': {
        token->type = GeTomlTokenType_Dot;
        token->value = (HeliosStringView) { .data = s->data + s->byte_offset, .count = 1 };
        return 1;
    }
    case '"': {
        UZ start = s->byte_offset + 1;
        while (HeliosString8StreamNext(s, &cur_char)) {
            if (cur_char == '"') break;
        }
        UZ end = s->byte_offset;

        if (s->byte_offset >= s->count) {
            token->type = GeTomlTokenType_UnterminatedString;
            token->value = (HeliosStringView) { .data = s->data + start, .count = end - start };
            return 1;
        }

        token->type = GeTomlTokenType_String;
        token->value = (HeliosStringView) { .data = s->data + start, .count = end - start };

        return 1;
    }
    case '\'': HELIOS_PANIC("Literal strings aren't supported yet");
    case '\r': {
        if (!HeliosString8StreamNext(s, &cur_char)) {
            token->type = GeTomlTokenType_Illegal;
            token->value = (HeliosStringView) { .data = s->data + s->byte_offset, .count = 1 };
            return 1;
        }

        if (cur_char != '\n') {
            HeliosString8StreamRetreat(s);
            token->type = GeTomlTokenType_Illegal;
            token->value = (HeliosStringView) { .data = s->data + s->byte_offset, .count = 1 };
            return 1;
        }

        token->type = GeTomlTokenType_Newline;
        token->value = (HeliosStringView) { .data = s->data + s->byte_offset - 1, .count = 2 };
        return 1;
    }
    case '#': {
        while (HeliosString8StreamNext(s, &cur_char) && cur_char != '\n');
        return _GeTomlNextToken(s, token);
    }
    default: {
        UZ start = s->byte_offset;

        if (HeliosCharIsDigit(cur_char)) {
            U8 base = 10;

            if (HeliosString8StreamNext(s, &cur_char) && !HeliosCharIsDigit(cur_char)) {
                if (cur_char == 'b')      base = 2;
                else if (cur_char == 'o') base = 8;
                else if (cur_char == 'x') base = 16;
                else goto make_token;
            }

            while (HeliosString8StreamNext(s, &cur_char)) {
                if (!GeIsCharDigitInBase(cur_char, base)) break;
            }

        make_token:

            UZ end = s->byte_offset;

            token->type = GeTomlTokenType_Int;
            token->value = (HeliosStringView) { .data = s->data + start, .count = end - start };

            HeliosString8StreamRetreat(s);

            return 1;
        }

        HELIOS_VERIFY(HeliosCharIsAlpha(cur_char));

        while (HeliosString8StreamNext(s, &cur_char)) {
            if (!_GeTomlIsValidIdentChar(cur_char)) break;
        }

        UZ end = s->byte_offset;

        token->type = GeTomlTokenType_Identifier;
        token->value = (HeliosStringView) { .data = s->data + start, .count = end - start };

        HeliosString8StreamRetreat(s);

        return 1;
    }
    }
}

#define GE_TOML_BAIL_ON_STREAM(ctx, msg) do {                           \
        GeSourceLocation source_location = _GeSourceLocationFromStream(&(ctx).stream); \
        snprintf((ctx).err_buf, (ctx).err_buf_count, "%d:%d: " msg, source_location.line, source_location.column); \
        return 0;                                                       \
    } while (0)

#define GE_TOML_BAIL_ON_STREAM_FMT(ctx, fmt, ...)  do {                 \
        GeSourceLocation source_location = _GeSourceLocationFromStream(&(ctx).stream); \
        snprintf((ctx).err_buf, (ctx).err_buf_count, "%d:%d: " fmt, source_location.line, source_location.column, __VA_ARGS__); \
        return 0;                                                       \
    } while (0)

#define GE_TOML_BAIL_ON_TOKEN(ctx, token, msg) do {                     \
        GeSourceLocation source_location = _GeSourceLocationFromTokenData(&(ctx).stream, (token).value); \
        snprintf((ctx).err_buf, (ctx).err_buf_count, "%d:%d: " msg, source_location.line, source_location.column); \
        return 0;                                                       \
    } while (0)

#define GE_TOML_BAIL_ON_TOKEN_FMT(ctx, token, fmt, ...)  do {           \
        GeSourceLocation source_location = _GeSourceLocationFromToken((token)); \
        snprintf((ctx).err_buf, (ctx).err_buf_count, "%d:%d: " fmt, source_location.line, source_location.column, __VA_ARGS__); \
        return 0;                                                       \
    } while (0)

#define GE_TOML_NEXT_TOKEN_OR_BAIL(ctx, token) do {             \
        if (!_GeTomlNextToken(&(ctx).stream, &(token))) {       \
            HeliosString8StreamRetreat(&(ctx).stream);          \
            GE_TOML_BAIL_ON_STREAM((ctx), "unexpected EOF");    \
        }                                                       \
    } while (0)

#define GE_TOML_PEEK_TOKEN_OR_BAIL(ctx, token) do {             \
        if (!_GeTomlPeekToken(&(ctx).stream, &(token))) {       \
            HeliosString8StreamRetreat(&(ctx).stream);          \
            GE_TOML_BAIL_ON_STREAM((ctx), "unexpected EOF");    \
        }                                                       \
    } while (0)

HELIOS_INTERNAL B32 _GeTomlPeekToken(HeliosString8Stream *s, GeTomlToken *token) {
    UZ byte_offset = s->byte_offset;
    UZ char_offset = s->char_offset;
    if (!_GeTomlNextToken(s, token)) return 0;

    s->byte_offset = byte_offset;
    s->char_offset = char_offset;

    return 1;
}

HELIOS_INTERNAL HELIOS_INLINE void _GeTomlAdvanceTokens(HeliosString8Stream *s) {
    GeTomlToken tok;
    _GeTomlNextToken(s, &tok);
}

#ifdef ASTRON_ERMIS_H
    ERMIS_IMPL_ARRAY(GeTomlValue, GeTomlArray)

    ERMIS_DECL_ARRAY(HeliosStringView, GeTomlKey)
    ERMIS_IMPL_ARRAY(HeliosStringView, GeTomlKey)
#define TOML_ARRAY_GROW_FACTOR ERMIS_ARRAY_GROW_FACTOR
#else
#define TOML_ARRAY_GROW_FACTOR(x) ((((x) + 1) * 3) >> 1)

HELIOS_INLINE void GeTomlArrayInit(GeTomlArray *arr, HeliosAllocator allocator, UZ cap) {
    arr->items = (GeTomlValue *)HeliosAlloc(allocator, sizeof(GeTomlValue) * cap);
    arr->capacity = cap;
    arr->count = 0;
    arr->allocator = allocator;
}

HELIOS_INLINE void GeTomlArrayPush(GeTomlArray *arr, GeTomlValue item) {
    if (arr->count >= arr->capacity) {
        UZ new_capacity = TOML_ARRAY_GROW_FACTOR(arr->capacity);
        arr->items = HeliosRealloc(arr->allocator, arr->items, sizeof(GeTomlValue) * arr->capacity, sizeof(GeTomlValue) * new_capacity);
        arr->capacity = new_capacity;
    }

    arr->items[arr->count++] = item;
}

HELIOS_INLINE GeTomlValue GeTomlArrayPop(GeTomlArray *arr) {
    HELIOS_VERIFY(arr->count != 0);
    return arr->items[--arr->count];
}

HELIOS_INLINE GeTomlValue GeTomlArrayAt(GeTomlArray *arr, UZ idx) {
    HELIOS_VERIFY(idx < arr->count);
    return arr->items[idx];
}

HELIOS_INLINE GeTomlValue *GeTomlArrayAtP(GeTomlArray *arr, UZ idx) {
    HELIOS_VERIFY(idx < arr->count);
    return &arr->items[idx];
}

HELIOS_INLINE const GeTomlValue *GeTomlArrayAtPC(const GeTomlArray *arr, UZ idx) {
    HELIOS_VERIFY(idx < arr->count);
    return &arr->items[idx];
}

HELIOS_INLINE void GeTomlArrayFree(GeTomlArray *arr) {
    HeliosFree(arr->allocator, arr->items, sizeof(GeTomlValue) * arr->capacity);
}
typedef struct GeTomlKey {
    HeliosStringView *items;
    UZ count;
    UZ capacity;
    HeliosAllocator allocator;
} GeTomlKey;

HELIOS_INTERNAL void GeTomlKeyInit(HeliosAllocator allocator, GeTomlKey *key, UZ cap) {
    key->items = HeliosAlloc(allocator, sizeof(HeliosStringView) * cap);
    key->count = 0;
    key->capacity = cap;
    key->allocator = allocator;
}

HELIOS_INTERNAL void GeTomlKeyPush(GeTomlKey *key, HeliosStringView part) {
    if (key->capacity >= key->count) {
        UZ new_cap = TOML_ARRAY_GROW_FACTOR(key->capacity);
        key->items = (HeliosStringView *)HeliosRealloc(key->allocator, key->items, sizeof(HeliosStringView) * key->capacity, sizeof(HeliosStringView) * new_cap);
        key->capacity = new_cap;
    }

    key->items[key->count++] = part;
}
#endif // ASTRON_ERMIS_H

// TODO: support quoted keys
HELIOS_INTERNAL B32 _GeTomlParseKey(GeTomlParsingContext *ctx, GeTomlKey *out_key) {
    GeTomlToken cur_token;

    GeTomlKeyInit(ctx->allocator, out_key, 4);

    do {
        GE_TOML_NEXT_TOKEN_OR_BAIL(*ctx, cur_token);

        if (cur_token.type != GeTomlTokenType_Identifier) {
            GE_TOML_BAIL_ON_TOKEN(*ctx, cur_token, "expected an identifier");
        }

        UZ key_part_count = cur_token.value.count;
        U8 *key_part_data = (U8 *)HeliosAlloc(ctx->allocator, key_part_count);
        memcpy(key_part_data, cur_token.value.data, key_part_count);

        HeliosStringView key_part = { .count = key_part_count, .data = key_part_data };

        GeTomlKeyPush(out_key, key_part);

        if (!_GeTomlPeekToken(&ctx->stream, &cur_token) || cur_token.type != GeTomlTokenType_Dot) break;
        _GeTomlAdvanceTokens(&ctx->stream);
    } while (1);

    return 1;
}

HELIOS_INTERNAL GeTomlValue *_GeTomlTableInsert(HeliosAllocator allocator, GeTomlTable *table, HeliosStringView key, GeTomlValue value) {
    // Check if the current table node is empty, and use it in that case.
    if (table->key.data == NULL) {
        table->key = key;
        table->value = value;
        return &table->value;
    }

    for (; table->next != NULL; table = table->next);

    table->next = (GeTomlTable *)HeliosAlloc(allocator, sizeof(GeTomlTable));
    table->next->key = key;
    table->next->value = value;

    return &table->next->value;
}

HELIOS_INTERNAL GeTomlValue *_GeTomlTableInsertKey(GeTomlParsingContext *ctx,
                                                   GeTomlTable *table,
                                                   GeTomlKey key,
                                                   GeTomlValue value) {
    HELIOS_ASSERT(key.count > 0);

    HeliosStringView leaf_key = key.items[key.count - 1];

    GeTomlTable *cur_table = table;

    for (UZ i = 0; i < key.count - 1; ++i) {
        HeliosStringView subtable_name = key.items[i];
        GeTomlTable *subtable;

        GeTomlValue *existing_subtable_value = GeTomlTableFindSV(cur_table, subtable_name);
        if (existing_subtable_value) {
            if (existing_subtable_value->type != GeTomlValueType_Table) {
                GE_TOML_BAIL_ON_STREAM_FMT(*ctx, "expected key '" HELIOS_SV_FMT "' to refer to a table", HELIOS_SV_ARG(subtable_name));
            }

            subtable = existing_subtable_value->t;
        } else {
            subtable = (GeTomlTable *)HeliosAlloc(ctx->allocator, sizeof(GeTomlTable));
            GeTomlValue subtable_value = {
                .type = GeTomlValueType_Table,
                .t = subtable,
            };
            _GeTomlTableInsert(ctx->allocator, cur_table, subtable_name, subtable_value);
        }

        cur_table = subtable;
    }

    GeTomlValue *existing_value_for_key = GeTomlTableFindSV(cur_table, leaf_key);
    if (existing_value_for_key != NULL) {
        GE_TOML_BAIL_ON_STREAM_FMT(*ctx, "cannot redefine key '" HELIOS_SV_FMT "'", HELIOS_SV_ARG(leaf_key));
    }

    return _GeTomlTableInsert(ctx->allocator, cur_table, leaf_key, value);
}

HELIOS_INTERNAL B32 _GeTomlParseKeyValue(GeTomlParsingContext *ctx, GeTomlKey *key, GeTomlValue *value);

HELIOS_INTERNAL B32 _GeTomlParseValue(GeTomlParsingContext *ctx, GeTomlValue *out) {
    GeTomlToken cur_token;
    GE_TOML_NEXT_TOKEN_OR_BAIL(*ctx, cur_token);

    switch (cur_token.type) {
    case GeTomlTokenType_String: {
        HeliosStringView s = HeliosStringViewClone(ctx->allocator, cur_token.value);

        *out = (GeTomlValue) {
            .type = GeTomlValueType_String,
            .s = s,
        };
        return 1;
    }
    case GeTomlTokenType_Int: {
        HeliosStringView int_value = cur_token.value;
        U8 base = 10;
        if (HeliosStringViewStartsWith(cur_token.value, "0x")) {
            int_value.data += 2;
            int_value.count -= 2;
            base = 16;
        } else if (HeliosStringViewStartsWith(cur_token.value, "0o")) {
            int_value.data += 2;
            int_value.count -= 2;
            base = 8;
        } else if (HeliosStringViewStartsWith(cur_token.value, "0b")) {
            int_value.data += 2;
            int_value.count -= 2;
            base = 2;
        }

        S64 i;
        HELIOS_ASSERT(HeliosParseS64(int_value, base, &i));

        *out = (GeTomlValue) {
            .type = GeTomlValueType_Int,
            .i = i,
        };
        return 1;
    }
    case GeTomlTokenType_Identifier: {
        if (HeliosStringViewEqualCStr(cur_token.value, "true")) {
            *out = (GeTomlValue) {
                .type = GeTomlValueType_Bool,
                .b = 1,
            };
            return 1;
        }

        if (HeliosStringViewEqualCStr(cur_token.value, "false")) {
            *out = (GeTomlValue) {
                .type = GeTomlValueType_Bool,
                .b = 0,
            };
            return 1;
        }

        GE_TOML_BAIL_ON_STREAM(*ctx, "unexpected identifier");
    }
    case GeTomlTokenType_Float: {
        F64 f;
        HELIOS_ASSERT(HeliosParseF64(cur_token.value, &f));

        *out = (GeTomlValue) {
            .type = GeTomlValueType_Float,
            .f = f,
        };
        return 1;
    }
    case GeTomlTokenType_LeftBracket: {
        GeTomlArray array;
        GeTomlArrayInit(&array, ctx->allocator, 15);

        while (1) {
            GE_TOML_PEEK_TOKEN_OR_BAIL(*ctx, cur_token);

            if (cur_token.type == GeTomlTokenType_RightBracket) {
                _GeTomlAdvanceTokens(&ctx->stream);
                break;
            }

            GeTomlValue arr_elem;
            if (!_GeTomlParseValue(ctx, &arr_elem)) return 0;
            GeTomlArrayPush(&array, arr_elem);

            GE_TOML_PEEK_TOKEN_OR_BAIL(*ctx, cur_token);

            if (cur_token.type == GeTomlTokenType_Comma) {
                _GeTomlAdvanceTokens(&ctx->stream);
            }
        }

        *out = (GeTomlValue) {
            .type = GeTomlValueType_Array,
            .a = array,
        };
        return 1;
    }
    case GeTomlTokenType_LeftBrace: {
        GeTomlTable *table = (GeTomlTable *)HeliosAlloc(ctx->allocator, sizeof(GeTomlTable));

        while (1) {
            if (cur_token.type == GeTomlTokenType_RightBrace) {
                _GeTomlAdvanceTokens(&ctx->stream);
                break;
            }

            GeTomlKey key;
            GeTomlValue value;
            if (!_GeTomlParseKeyValue(ctx, &key, &value)) return 0;

            if (_GeTomlTableInsertKey(ctx,
                                      table,
                                      key,
                                      value) == NULL) return 0;

            GE_TOML_PEEK_TOKEN_OR_BAIL(*ctx, cur_token);

            if (cur_token.type == GeTomlTokenType_Comma) {
                _GeTomlAdvanceTokens(&ctx->stream);
            } else {
                break;
            }
        }

        GE_TOML_NEXT_TOKEN_OR_BAIL(*ctx, cur_token);

        if (cur_token.type != GeTomlTokenType_RightBrace) {
            GE_TOML_BAIL_ON_TOKEN(*ctx, cur_token, "expected '}'");
        }

        *out = (GeTomlValue) {
            .type = GeTomlValueType_Table,
            .t = table,
        };
        return 1;
    }
    default: GE_TOML_BAIL_ON_TOKEN(*ctx, cur_token, "expected an expression");
    }
}

HELIOS_INTERNAL B32 _GeTomlParseKeyValue(GeTomlParsingContext *ctx, GeTomlKey *out_key, GeTomlValue *out_value) {
    if (!_GeTomlParseKey(ctx, out_key)) return 0;

    GeTomlToken cur_token;

    GE_TOML_NEXT_TOKEN_OR_BAIL(*ctx, cur_token);

    if (cur_token.type != GeTomlTokenType_Equals) {
        GE_TOML_BAIL_ON_TOKEN(*ctx, cur_token, "expected '='");
    }

    if (!_GeTomlParseValue(ctx, out_value)) return 0;

    return 1;
}

HELIOS_DEF GeTomlTable *GeTomlParseBuffer(HeliosAllocator allocator,
                                          const char *buf,
                                          UZ buf_count,
                                          char *err_buf,
                                          UZ err_buf_count) {
    GeTomlParsingContext ctx;
    HeliosString8StreamInit(&ctx.stream, (const U8 *)buf, buf_count);
    ctx.err_buf = err_buf;
    ctx.err_buf_count = err_buf_count;
    ctx.allocator = allocator;

    GeTomlTable *root_table = (GeTomlTable *)HeliosAlloc(ctx.allocator, sizeof(GeTomlTable));

    GeTomlTable *current_table = root_table;

    GeTomlToken cur_token;
    while (_GeTomlNextToken(&ctx.stream, &cur_token)) {
        switch (cur_token.type) {
        case GeTomlTokenType_LeftBracket: {
            GeTomlKey child_table_key;

            if (!_GeTomlParseKey(&ctx, &child_table_key)) return NULL;

            GE_TOML_NEXT_TOKEN_OR_BAIL(ctx, cur_token);

            if (cur_token.type != GeTomlTokenType_RightBracket) {
                GE_TOML_BAIL_ON_TOKEN(ctx, cur_token, "expected ']'");
            }

            if (_GeTomlNextToken(&ctx.stream, &cur_token) && cur_token.type != GeTomlTokenType_Newline) {
                GE_TOML_BAIL_ON_TOKEN(ctx, cur_token, "expected a newline");
            }

            GeTomlTable *child_table = (GeTomlTable *)HeliosAlloc(ctx.allocator, sizeof(GeTomlTable));

            GeTomlValue child_table_value = {
                .type = GeTomlValueType_Table,
                .t = child_table,
            };

            GeTomlValue *child_table_value_in_table = _GeTomlTableInsertKey(&ctx,
                                                                            root_table,
                                                                            child_table_key,
                                                                            child_table_value);
            if (child_table_value_in_table == NULL) return NULL;
            HELIOS_ASSERT(child_table_value_in_table->type == GeTomlValueType_Table);
            current_table = child_table_value_in_table->t;
            break;
        }
        case GeTomlTokenType_Newline: break;
        default: {
            if (cur_token.type != GeTomlTokenType_Identifier) GE_TOML_BAIL_ON_TOKEN(ctx, cur_token, "expected an identifier");

            HeliosStringView key = HeliosStringViewClone(allocator, cur_token.value);

            GE_TOML_NEXT_TOKEN_OR_BAIL(ctx, cur_token);

            if (cur_token.type != GeTomlTokenType_Equals) GE_TOML_BAIL_ON_TOKEN(ctx, cur_token, "expected '='");

            GeTomlValue value;
            if (!_GeTomlParseValue(&ctx, &value)) return NULL;

            if (_GeTomlNextToken(&ctx.stream, &cur_token) && cur_token.type != GeTomlTokenType_Newline) {
                GE_TOML_BAIL_ON_TOKEN(ctx, cur_token, "expected a newline");
            }

            _GeTomlTableInsert(ctx.allocator, current_table, key, value);
            break;
        }
        }
    }

    return root_table;
}

#endif // ASTRON_GE_USE_TOML
#endif // ASTRON_GE_IMPLEMENTATION

#endif // ASTRON_GE_H
