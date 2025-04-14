// TODOs:
// toml:
//   create a basic context structure to not pass 10 arguments to each procedure
//   add spec conformance tests
//   implement error location procedures

#ifndef ASTRON_GE_H
#define ASTRON_GE_H

#ifndef ASTRON_HELIOS_H
#    error "'ge.h' requires 'helios.h' to be included first"
#endif // ASTRON_HELIOS_H

#ifdef ASTRON_HELIOS_IMPLEMENTATION
#    define ASTRON_GE_IMPLEMENTATION
#endif // ASTRON_HELIOS_IMPLEMENTATION

// common definitions

typedef struct GeSourceLocation {
    U32 line;
    U32 column;
} GeSourceLocation;

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
        HeliosString8 s;
        GeTomlArray a;
        GeTomlTable *t;
    };
};

struct GeTomlTable {
    struct GeTomlTable *next;
    HeliosStringView key;
    GeTomlValue value;
};

GeTomlTable *GeTomlParseBuffer(HeliosAllocator allocator,
                               const char *buf,
                               UZ buf_count,
                               char *err_buf,
                               UZ err_buf_count);

GeTomlValue *GeTomlTableFind(GeTomlTable *table, HeliosStringView key);
#endif // ASTRON_GE_USE_TOML

#ifdef ASTRON_GE_IMPLEMENTATION

HELIOS_INTERNAL GeSourceLocation _GeSourceLocationFromStream(HeliosString8Stream *stream) {
    HELIOS_UNUSED(stream);
    HELIOS_TODO();
}

HELIOS_INTERNAL GeSourceLocation _GeSourceLocationFromTokenData(HeliosString8Stream *stream, HeliosStringView data) {
    HELIOS_UNUSED(stream);
    HELIOS_UNUSED(data);
    HELIOS_TODO();
}

#ifdef ASTRON_GE_USE_TOML

GeTomlValue *GeTomlTableFind(GeTomlTable *table, HeliosStringView key) {
    HELIOS_VERIFY(table != NULL);
    for (; table != NULL; table = table->next) {
        if (HeliosStringViewEqual(table->key, key)) return &table->value;
    }

    return NULL;
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

        HELIOS_ASSERT(s->byte_offset < s->count);

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
    default: {
        UZ start = s->byte_offset;

        if (HeliosCharIsDigit(cur_char)) {
            while (HeliosString8StreamNext(s, &cur_char)) {
                if (!HeliosCharIsDigit(cur_char)) break;
            }

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
#else

#endif // ASTRON_ERMIS_H

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
#endif // ASTRON_HELIOS_H

// TODO: support quoted keys
HELIOS_INTERNAL B32 _GeTomlParseKey(HeliosAllocator allocator,
                                    HeliosString8Stream *input_stream,
                                    char *err_buf,
                                    UZ err_buf_count,
                                    GeTomlKey *out_key) {
    GeTomlToken cur_token;

    GeTomlKeyInit(allocator, out_key, 4);

    do {
        if (!_GeTomlNextToken(input_stream, &cur_token)) {
            GeSourceLocation source_location = _GeSourceLocationFromStream(input_stream);
            snprintf(err_buf, err_buf_count, "%d:%d: unexpected EOF", source_location.line, source_location.column);
            return 0;
        }

        if (cur_token.type != GeTomlTokenType_Identifier) {
            GeSourceLocation source_location = _GeSourceLocationFromTokenData(input_stream, cur_token.value);
            snprintf(err_buf, err_buf_count, "%d:%d: expected an identifier", source_location.line, source_location.column);
            return 0;
        }

        UZ key_part_count = cur_token.value.count;
        U8 *key_part_data = (U8 *)HeliosAlloc(allocator, key_part_count);
        memcpy(key_part_data, cur_token.value.data, key_part_count);

        HeliosStringView key_part = { .count = key_part_count, .data = key_part_data };

        GeTomlKeyPush(out_key, key_part);

        if (!_GeTomlPeekToken(input_stream, &cur_token) || cur_token.type != GeTomlTokenType_Dot) break;
        _GeTomlAdvanceTokens(input_stream);
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

HELIOS_INTERNAL GeTomlValue *_GeTomlTableInsertKey(HeliosAllocator allocator,
                                                   GeTomlTable *table,
                                                   GeTomlKey key,
                                                   GeTomlValue value,
                                                   HeliosString8Stream *input_stream,
                                                   char *err_buf,
                                                   UZ err_buf_count) {
    HELIOS_ASSERT(key.count > 0);

    HeliosStringView leaf_key = key.items[key.count - 1];

    GeTomlTable *cur_table = table;

    for (UZ i = 0; i < key.count - 1; ++i) {
        HeliosStringView subtable_name = key.items[i];
        GeTomlTable *subtable;

        GeTomlValue *existing_subtable_value = GeTomlTableFind(cur_table, subtable_name);
        if (existing_subtable_value) {
            if (existing_subtable_value->type != GeTomlValueType_Table) {
                GeSourceLocation err_location = _GeSourceLocationFromStream(input_stream);
                snprintf(err_buf,
                         err_buf_count,
                         "%d:%d: expected key '" HELIOS_SV_FMT "' to refer to a table",
                         err_location.line,
                         err_location.column,
                         HELIOS_SV_ARG(subtable_name));
                return NULL;
            }

            subtable = existing_subtable_value->t;
        } else {
            subtable = (GeTomlTable *)HeliosAlloc(allocator, sizeof(GeTomlTable));
            GeTomlValue subtable_value = {
                .type = GeTomlValueType_Table,
                .t = subtable,
            };
            _GeTomlTableInsert(allocator, cur_table, subtable_name, subtable_value);
        }

        cur_table = subtable;
    }

    GeTomlValue *existing_value_for_key = GeTomlTableFind(cur_table, leaf_key);
    if (existing_value_for_key != NULL) {
        GeSourceLocation err_location = _GeSourceLocationFromStream(input_stream);
        snprintf(err_buf,
                 err_buf_count,
                 "%d:%d: cannot redefine key '" HELIOS_SV_FMT "'",
                 err_location.line,
                 err_location.column,
                 HELIOS_SV_ARG(leaf_key));
        return NULL;
    }

    return _GeTomlTableInsert(allocator, cur_table, leaf_key, value);
}

HELIOS_INTERNAL B32 _GeTomlParseKeyValue(HeliosAllocator allocator, HeliosString8Stream *stream, GeTomlKey *key, GeTomlValue *value, char *err_buf, UZ err_buf_count);

HELIOS_INTERNAL B32 _GeTomlParseValue(HeliosAllocator allocator, HeliosString8Stream *stream, GeTomlValue *out, char *err_buf, UZ err_buf_count) {
    GeTomlToken cur_token;
    if (!_GeTomlNextToken(stream, &cur_token)) {
        GeSourceLocation err_location = _GeSourceLocationFromStream(stream);
        snprintf(err_buf, err_buf_count, "%d:%d: unexpected EOF", err_location.line, err_location.column);
        return 0;
    }

    switch (cur_token.type) {
    case GeTomlTokenType_String: {
        HeliosString8 s = HeliosString8FromStringView(allocator, cur_token.value);
        *out = (GeTomlValue) {
            .type = GeTomlValueType_String,
            .s = s,
        };
        return 1;
    }
    case GeTomlTokenType_Int: {
        S64 i;
        HELIOS_ASSERT(HeliosParseS64(cur_token.value, &i));

        *out = (GeTomlValue) {
            .type = GeTomlValueType_String,
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

        GeSourceLocation err_location = _GeSourceLocationFromStream(stream);
        snprintf(err_buf, err_buf_count, "%d:%d: unexpected identifier", err_location.line, err_location.column);
        return 0;
    }
    case GeTomlTokenType_Float: {
        F64 f;
        HELIOS_ASSERT(HeliosParseF64(cur_token.value, &f));

        *out = (GeTomlValue) {
            .type = GeTomlValueType_String,
            .f = f,
        };
        return 1;
    }
    case GeTomlTokenType_LeftBracket: {
        GeTomlArray array;
        GeTomlArrayInit(&array, allocator, 15);

        while (1) {
            if (!_GeTomlPeekToken(stream, &cur_token)) {
                GeSourceLocation err_location = _GeSourceLocationFromStream(stream);
                snprintf(err_buf, err_buf_count, "%d:%d: unexpected EOF", err_location.line, err_location.column);
                return 0;
            }

            if (cur_token.type == GeTomlTokenType_RightBracket) {
                _GeTomlAdvanceTokens(stream);
                break;
            }

            GeTomlValue arr_elem;
            if (!_GeTomlParseValue(allocator, stream, &arr_elem, err_buf, err_buf_count)) return 0;
            GeTomlArrayPush(&array, arr_elem);

            if (!_GeTomlPeekToken(stream, &cur_token)) {
                GeSourceLocation err_location = _GeSourceLocationFromStream(stream);
                snprintf(err_buf, err_buf_count, "%d:%d: unexpected EOF", err_location.line, err_location.column);
                return 0;
            }

            if (cur_token.type == GeTomlTokenType_Comma) {
                _GeTomlAdvanceTokens(stream);
            }
        }

        *out = (GeTomlValue) {
            .type = GeTomlValueType_Array,
            .a = array,
        };
        return 1;
    }
    case GeTomlTokenType_LeftBrace: {
        GeTomlTable *table = (GeTomlTable *)HeliosAlloc(allocator, sizeof(GeTomlTable));

        while (1) {
            if (cur_token.type == GeTomlTokenType_RightBrace) {
                _GeTomlAdvanceTokens(stream);
                break;
            }

            GeTomlKey key;
            GeTomlValue value;
            if (!_GeTomlParseKeyValue(allocator, stream, &key, &value, err_buf, err_buf_count)) return 0;

            if (_GeTomlTableInsertKey(allocator,
                                      table,
                                      key,
                                      value,
                                      stream,
                                      err_buf,
                                      err_buf_count) == NULL) return 0;

            if (!_GeTomlPeekToken(stream, &cur_token)) {
                GeSourceLocation err_location = _GeSourceLocationFromStream(stream);
                snprintf(err_buf, err_buf_count, "%d:%d: unexpected EOF", err_location.line, err_location.column);
                return 0;
            }

            if (cur_token.type == GeTomlTokenType_Comma) {
                _GeTomlAdvanceTokens(stream);
            } else {
                break;
            }
        }

        if (!_GeTomlNextToken(stream, &cur_token)) {
            GeSourceLocation err_location = _GeSourceLocationFromStream(stream);
            snprintf(err_buf, err_buf_count, "%d:%d: unexpected EOF", err_location.line, err_location.column);
            return 0;
        }

        if (cur_token.type != GeTomlTokenType_RightBrace) {
            GeSourceLocation err_location = _GeSourceLocationFromStream(stream);
            snprintf(err_buf, err_buf_count, "%d:%d: expected '}'", err_location.line, err_location.column);
            return 0;
        }

        *out = (GeTomlValue) {
            .type = GeTomlValueType_Table,
            .t = table,
        };
        return 1;
    }
    default: HELIOS_TODO();
    }
}

HELIOS_INTERNAL B32 _GeTomlParseKeyValue(HeliosAllocator allocator, HeliosString8Stream *stream, GeTomlKey *out_key, GeTomlValue *out_value, char *err_buf, UZ err_buf_count) {
    if (!_GeTomlParseKey(allocator, stream, err_buf, err_buf_count, out_key)) return 0;

    GeTomlToken cur_token;

    if (!_GeTomlNextToken(stream, &cur_token)) {
        GeSourceLocation err_location = _GeSourceLocationFromStream(stream);
        snprintf(err_buf, err_buf_count, "%d:%d: unexpected EOF", err_location.line, err_location.column);
        return 0;
    }

    if (cur_token.type != GeTomlTokenType_Equals) {
        GeSourceLocation err_location = _GeSourceLocationFromStream(stream);
        snprintf(err_buf, err_buf_count, "%d:%d: expected '='", err_location.line, err_location.column);
        return 0;
    }

    if (!_GeTomlParseValue(allocator, stream, out_value, err_buf, err_buf_count)) return 0;

    return 1;
}

GeTomlTable *GeTomlParseBuffer(HeliosAllocator allocator,
                      const char *buf,
                      UZ buf_count,
                      char *err_buf,
                      UZ err_buf_count) {
    HeliosString8Stream input_stream;
    HeliosString8StreamInit(&input_stream, (const U8 *)buf, buf_count);

    GeTomlTable *root_table = (GeTomlTable *)HeliosAlloc(allocator, sizeof(GeTomlTable));

    GeTomlTable *current_table = root_table;

    GeTomlToken cur_token;
    while (_GeTomlNextToken(&input_stream, &cur_token)) {
        switch (cur_token.type) {
        case GeTomlTokenType_LeftBracket: {
            GeTomlKey child_table_key;

            if (!_GeTomlParseKey(allocator, &input_stream, err_buf, err_buf_count, &child_table_key)) return NULL;

            if (!_GeTomlNextToken(&input_stream, &cur_token)) {
                GeSourceLocation err_location = _GeSourceLocationFromStream(&input_stream);
                snprintf(err_buf, err_buf_count, "%d:%d: unexpected EOF", err_location.line, err_location.column);
                return NULL;
            }

            if (cur_token.type != GeTomlTokenType_RightBracket) {
                GeSourceLocation err_location = _GeSourceLocationFromStream(&input_stream);
                snprintf(err_buf, err_buf_count, "%d:%d: expected ']'", err_location.line, err_location.column);
                return NULL;
            }

            if (_GeTomlNextToken(&input_stream, &cur_token) && cur_token.type != GeTomlTokenType_Newline) {
                GeSourceLocation err_location = _GeSourceLocationFromStream(&input_stream);
                snprintf(err_buf, err_buf_count, "%d:%d: expected a newline", err_location.line, err_location.column);
                return NULL;
            }

            // TODO: Check if the table was already defined.
            GeTomlTable *child_table = (GeTomlTable *)HeliosAlloc(allocator, sizeof(GeTomlTable));

            GeTomlValue child_table_value = {
                .type = GeTomlValueType_Table,
                .t = child_table,
            };

            GeTomlValue *child_table_value_in_table = _GeTomlTableInsertKey(allocator,
                                                                            root_table,
                                                                            child_table_key,
                                                                            child_table_value,
                                                                            &input_stream,
                                                                            err_buf,
                                                                            err_buf_count);
            if (child_table_value_in_table == NULL) return NULL;
            HELIOS_ASSERT(child_table_value_in_table->type == GeTomlValueType_Table);
            current_table = child_table_value_in_table->t;
            break;
        }
        default: {
            HELIOS_VERIFY(cur_token.type == GeTomlTokenType_Identifier);
            HeliosString8 key = HeliosString8FromSV(allocator, cur_token.value);

            if (!_GeTomlNextToken(&input_stream, &cur_token)) {
                GeSourceLocation err_location = _GeSourceLocationFromStream(&input_stream);
                snprintf(err_buf, err_buf_count, "%d:%d: unexpected EOF", err_location.line, err_location.column);
                return NULL;
            }

            if (cur_token.type != GeTomlTokenType_Equals) {
                GeSourceLocation err_location = _GeSourceLocationFromStream(&input_stream);
                snprintf(err_buf, err_buf_count, "%d:%d: expected '='", err_location.line, err_location.column);
                return NULL;
            }

            GeTomlValue value;
            if (!_GeTomlParseValue(allocator, &input_stream, &value, err_buf, err_buf_count)) return NULL;

            if (_GeTomlNextToken(&input_stream, &cur_token) && cur_token.type != GeTomlTokenType_Newline) {
                GeSourceLocation err_location = _GeSourceLocationFromStream(&input_stream);
                snprintf(err_buf, err_buf_count, "%d:%d: expected a newline", err_location.line, err_location.column);
                return NULL;
            }

            HeliosStringView key_view = HeliosString8View(key);

            // TODO: Check if the key is already present.
            _GeTomlTableInsert(allocator, current_table, key_view, value);
            break;
        }
        }
    }

    return root_table;
}

#endif // ASTRON_GE_USE_TOML
#endif // ASTRON_GE_IMPLEMENTATION

#endif // ASTRON_GE_H
