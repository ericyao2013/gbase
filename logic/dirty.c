#include <ctype.h>
#include <assert.h>

#include "base/hash.h"
#include "dirty.h"

// hash size
#define MAX_DIRTY_WORDS_HASH_COUNT MAX_DIRTY_WORDS_COUNT * 3

// if > 0x80, means double bytes
// else, single byte
const uint8_t GB_SPEC_CHAR = (uint8_t)(0x80);

typedef struct dirty_t {
    char word[MAX_DIRTY_WORDS_LEN];
    int prev;
    int next;
} dirty_t;

struct dirty_ctx_t {
    int table_size;
    dirty_t table[MAX_DIRTY_WORDS_COUNT];
    int hash[MAX_DIRTY_WORDS_HASH_COUNT];
    int index[256][256];
};

#define SET_DIRTY_FLAG(index, len)      (index |= (1 << len))
#define RESET_DIRTY_FLAG(index, len)    (index &= ~(1 << len))
#define CHECK_DIRTY_FLAG(index, len)    (index & (1 << len))

static dirty_ctx_t*
_dirty_init_table(const char* cfg) {
    if (!cfg) {
        return NULL;
    }
    dirty_ctx_t* ctx = (dirty_ctx_t*)MALLOC(sizeof(dirty_ctx_t));
    if (!ctx) {
        return NULL;
    }
    // open file
    FILE* fp = fopen(cfg, "r");
    if (!fp) {
        FREE(ctx);
        return NULL;
    }
    // read dirty words table
    ctx->table_size = 0;
    while (fgets(ctx->table[ctx->table_size].word, MAX_DIRTY_WORDS_LEN, fp)) {
        dirty_t* dirty = &ctx->table[ctx->table_size ++];
        // lower case
        for (size_t i = 0; i < strnlen(dirty->word, MAX_DIRTY_WORDS_LEN); ++ i) {
            dirty->word[i] = tolower((unsigned char)(dirty->word[i]));
        }
        dirty->next = dirty->prev = -1;
        // trim \r \n
        while (1) {
            int len = strnlen(dirty->word, MAX_DIRTY_WORDS_LEN);
            if (dirty->word[len - 1] == '\r' || dirty->word[len -1] == '\n') {
                dirty->word[len - 1] = 0;
            } else {
                break;
            }
        }
        // table full
        if (ctx->table_size >= MAX_DIRTY_WORDS_COUNT) {
            break;
        }
    }
    fclose(fp);
    fp = NULL;
    return ctx;
}

static int
_dirty_init_hash(dirty_ctx_t* ctx) {
    // build hash table
    memset(ctx->hash, -1, sizeof(ctx->hash));
    for (int i = 0; i < ctx->table_size; ++ i) {
        size_t len = strnlen(ctx->table[i].word, MAX_DIRTY_WORDS_LEN);
        uint32_t hkey = hash_jhash(ctx->table[i].word, len);
        int hindex = hkey % MAX_DIRTY_WORDS_HASH_COUNT;
        if (ctx->hash[hindex] < 0) {
            ctx->hash[hindex] = i;
        } else {
            int windex = ctx->hash[hindex];
            dirty_t* dirty = &ctx->table[windex];
            while (dirty->next >= 0) {
                windex = dirty->next;
                dirty = &ctx->table[windex];
            }
            dirty->next = i;
            ctx->table[i].prev = windex;
        }
    }
    return 0;
}

static int
_dirty_init_index(dirty_ctx_t* ctx) {
    memset(ctx->index, 0, sizeof(ctx->index));
    for (int i = 0; i < ctx->table_size; ++ i) {
        char* word = ctx->table[i].word;
        size_t len = strnlen(word, MAX_DIRTY_WORDS_LEN);
        if ((uint8_t)word[0] < GB_SPEC_CHAR) {
            SET_DIRTY_FLAG(ctx->index[0][(uint8_t)word[0]], len);
        } else if (len >= 2) {
            SET_DIRTY_FLAG(ctx->index[(uint8_t)word[0]][(uint8_t)word[1]], len);
        } else {
            return -1;
        }
    }
    return 0;
}

dirty_ctx_t*
dirty_create(const char* cfg) {
    dirty_ctx_t* ctx = _dirty_init_table(cfg);
    if (!ctx) {
        return NULL;
    }
    int ret = _dirty_init_hash(ctx);
    assert(0 == ret);
    ret = _dirty_init_index(ctx);
    assert(0 == ret);
    return ctx;
}

int
dirty_reload(dirty_ctx_t** ctx, const char* dirty_cfg) {
    dirty_ctx_t* new_ctx = dirty_create(dirty_cfg);
    if (!new_ctx) {
        return -1;
    }
    dirty_release(*ctx);
    *ctx = new_ctx;
    return 0;
}

int
dirty_hash_find(dirty_ctx_t* ctx, const char* src, size_t len) {
    if (!ctx || !src || len == 0) {
        return -1;
    }
    // index
    uint32_t hkey = hash_jhash(src, len);
    int hindex = hkey % MAX_DIRTY_WORDS_HASH_COUNT;
    if (ctx->index[hindex] < 0) {
        return -1;
    }
    // hash table
    int windex = ctx->hash[hindex];
    while (1) {
        // not found
        if (windex < 0) {
            return -1;
        }
        dirty_t* dirty = &ctx->table[windex];
        windex = dirty->next;
        if (0 == strncmp(dirty->word, src, len)) {
            return 0;
        }
    }
    return -1;
}

// return 0, success
// return -1, has dirty words
// return -2, encode error
// return -3, invalid input
int
dirty_check(dirty_ctx_t* ctx, const char* src, int len) {
    if (!ctx || !src || len > MAX_SOURCE_WORDS_LEN) {
        return -3;
    }
    static char lowcase[MAX_SOURCE_WORDS_LEN];
    for (int i = 0; i < len; ++ i) {
        lowcase[i] = tolower((unsigned char)src[i]);
    }
    for (int i = 0, step = 0; i < len; i += step) {
        int step, key = 0;
        const uint8_t* from = (const uint8_t*)&lowcase[i];
        if (from[0] < GB_SPEC_CHAR) {
            key = ctx->index[0][from[0]];
            step = 1;
        } else if (i + 1 < len) {
            key = ctx->index[from[0]][from[1]];
            step = 2;
        } else {
            printf("source code error\n");
            return -2;
        }
        // no index, go ahead
        if (0 == key) {
            continue;
        }
        // found key
        for (int k = 1; k < MAX_DIRTY_WORDS_LEN; ++ k) {
            // exceed len
            if (i + k > len) {
                break;
            }
            // no dirty
            if (0 == CHECK_DIRTY_FLAG(key, k)) {
                continue;
            }
            if (0 == dirty_hash_find(ctx, (const char*)from, k)) {
                return -1;
            }
            // no need to loop all
            RESET_DIRTY_FLAG(key, k);
            if (0 == key) {
                break;
            }
        }
    }
    return 0;
}

// return 0, check success and no need to replace
// return 1, check success and replace dirty words
// return -1, fail
int
dirty_replace(dirty_ctx_t* ctx, char* src, int len) {
    static char lowcase[MAX_SOURCE_WORDS_LEN];
    if (!ctx || !src || len > MAX_SOURCE_WORDS_LEN) {
        return -1;
    }
    for (int i = 0; i < len; ++ i) {
        lowcase[i] = tolower((unsigned char)src[i]);
    }
    for (int i = 0, step = 0; i < len; i += step) {
        int step, key = 0;
        uint8_t* from = (uint8_t*)&lowcase[i];
        if (from[0] < GB_SPEC_CHAR) {
            key = ctx->index[0][from[0]];
            step = 1;
        } else if (i + 1 < len) {
            key = ctx->index[from[0]][from[1]];
            step = 2;
        } else {
            printf("source code error\n");
            return -1;
        }
        // no index, go ahead
        if (0 == key) {
            continue;
        }
        // index exists, check
        for (int k = 1; k < MAX_DIRTY_WORDS_LEN; ++ k) {
            if (i + k > len) {
                break;
            }
            // no dirty
            if (0 == CHECK_DIRTY_FLAG(key, k)) {
                continue;
            }
            // look up table
            if (0 == dirty_hash_find(ctx, (const char*)from, k)) {
                for (int j = 0; j < k; ++ j) {
                    src[i + j] = DIRTY_REPLACE_CHAR;
                }
                step = k;
                break;
            }
            // no need to loop all
            RESET_DIRTY_FLAG(key, k);
            if (0 == key) {
                break;
            }
        }
    }
    return 0;
}

int
dirty_release(dirty_ctx_t* ctx) {
    if (ctx) {
        FREE(ctx);
        ctx = NULL;
    }
    return 0;
}

