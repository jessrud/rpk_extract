#include "rpack.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <assert.h>


// initialize rpk_archive structure from filesystem
rpk_archive *rpk_load(const char* filename) {
    
    rpk_archive temp;

    // abort if we cant open the file
    if ( ( temp.source = fopen(filename, "rb") ) == NULL )
        return NULL;

    // abort if we can't read the preamble
    if ( fread(&temp.header, sizeof ( rpk_preamble ), 1, temp.source) < 1 ) goto ERR;
    
    // abort if magic doesn't match
    if ( temp.header.magic != RPK_FILE_MAGIC ) goto ERR;


    // sanity check; header size should be greater than zero, and evenly divisible by the size of a header
    // entry.
    if ( ( temp.header.size == 0 ) ||
         ( ( temp.header.size % sizeof ( rpk_header_entry ) ) != 0 ) ) goto ERR;
         temp.entry_count = temp.header.size / sizeof ( rpk_header_entry );
     
    rpk_archive *result = malloc(sizeof ( rpk_archive ) + temp.header.size);

    // OOM
    if ( result == NULL ) goto ERR;

    //copy temp data over to newly allocated archive structure
    memcpy(result, &temp ,sizeof ( rpk_archive ));

    // read the header into the archive struct, and abort if we read the wrong number of entries.
    if ( fread(&result->header.entry, sizeof ( rpk_header_entry ), result->entry_count, temp.source) != result->entry_count )
        goto POST_ALLOC_ERR;

    return result;

POST_ALLOC_ERR:
    rpk_unload(result);
    return NULL;
ERR:
    fclose(temp.source);
    return NULL;
}

// read nbytes from source and write them to dest. nbytes should be less than or equal to BUFSIZ, and less than or equal size of readbuf
int pipe_bytes(FILE *source, FILE *dest, char *readbuf, size_t nbytes) {
    assert( nbytes <= BUFSIZ );
    int last_read = fread(readbuf, sizeof ( char ), nbytes, source);
            
    if ( last_read != nbytes ) return RPK_EXERR_READARCH;

    fflush(dest);
    int last_write = fwrite(readbuf, sizeof ( char ), nbytes, dest);

    if ( last_write != nbytes ) return RPK_EXERR_WRITEENT;
}

// expand archive entries to be files on disk. resulting files will have .rent file extension
int rpk_extract(rpk_archive* archive, const char* dest_dir) {
    int status = 0;
    char pwd[PATH_MAX];

    // offset into the archive file where the payload data begins
    const uint32_t payload_start = archive->header.size + sizeof ( rpk_preamble );

    if ( dest_dir != NULL ) {
        getcwd(pwd, PATH_MAX);

        if ( chdir(dest_dir) ) {
            status = RPK_EXERR_CDDEST;
            goto CLEANUP;
        };
    }
    
    for ( size_t i = 0; i < archive->entry_count; i++ ) {

        // construct an appropriate filename for the entry
        char namebuf[RPK_ENT_FNAME_SIZ];

        memset(namebuf, 0, RPK_ENT_FNAME_SIZ);
        strncpy(namebuf, archive->header.entry[i].name, RPK_ENT_NAME_SIZ);
        strncat(namebuf, RPK_ENT_FNAME_EXT, RPK_ENT_FNAME_EXT_SIZ + 1);

        // try to open output file for writing
        FILE *output = fopen(namebuf, "wb");

        if ( output == NULL ) {
            status = RPK_EXERR_WRITEENT;
            goto CLEANUP;
        }

        // abort if we cannot seek to the start of the entry payload in the file
        if ( fseek(archive->source, archive->header.entry[i].offset + payload_start, SEEK_SET) ) {
            status = RPK_EXERR_READARCH;
            goto CLEANUP;
        }

        char readbuf[BUFSIZ];
        int bytes_left = archive->header.entry[i].size;

        // write BUFSIZ sized blocks of data from payload to output file
        for ( ;bytes_left > BUFSIZ; bytes_left -= BUFSIZ ) {
            int result = pipe_bytes(archive->source, output, readbuf, BUFSIZ);
            if ( result < 0 ) {
                status = result;
                goto CLEANUP;
            }
            
        }
        // write any remaining data from payload
        if ( bytes_left > 0 ) {
            int result = pipe_bytes(archive->source, output, readbuf, bytes_left);
            if ( result < 0 ) {
                status = result;
                goto CLEANUP;
            }
        }
    }

CLEANUP:
    if ( dest_dir != NULL ) chdir(pwd);
    return status;

}

// teardown archive structure and free associated memory
void rpk_unload(rpk_archive* archive) {
    fclose(archive->source);
    memset(archive, 0, sizeof (rpk_archive));
    free(archive);
}
