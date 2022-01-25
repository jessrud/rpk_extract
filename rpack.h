#include <stdint.h>
#include <stdio.h>

#define RPK_FILE_MAGIC 0xAFBF0C01
#define RPK_ENT_NAME_SIZ 16
#define RPK_ENT_FNAME_EXT ".rent"
#define RPK_ENT_FNAME_EXT_SIZ 5
#define RPK_ENT_FNAME_SIZ RPK_ENT_NAME_SIZ + RPK_ENT_FNAME_EXT_SIZ

#define RPK_EXERR_READARCH -2
#define RPK_EXERR_WRITEENT -3
#define RPK_EXERR_CDDEST -4

// header entry in an rpk archive. file istle endian and structure assumes
// little endian host.
typedef struct {
    char name[RPK_ENT_NAME_SIZ];
    uint32_t offset;
    uint32_t size;
    char _pad0[8];

} rpk_header_entry;

typedef struct {
    uint32_t magic;
    uint32_t size;

} rpk_preamble;



// 'parsed' rpk archive header. payload is read from file as needed and file handle
// is retained for the life of the structure.
typedef struct {
    FILE *source;
    size_t entry_count;
    struct {
        rpk_preamble;
        rpk_header_entry entry[];

    } header;

} rpk_archive;



rpk_archive* rpk_load(const char* filename);
int rpk_extract(rpk_archive* archive, const char* dest_dir);
void rpk_unload(rpk_archive *archive);
