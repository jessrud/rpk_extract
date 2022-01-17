#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#include "rpack.h"


// initialize rpk_archive structure from filesystem
rpk_archive *rpk_load(const char* filename) {

    rpk_header_entry first_entry;
    FILE *source;

    // abort if we cant open the file
    if ( ( source = fopen(filename, "rb") ) == NULL )
        return NULL;

    // abort if theres is less than a single entry worth of data to read
    if ( fread(&first_entry, sizeof (rpk_header_entry), 1, source) < 1 ) goto ERR;
    
    // abort if magic doesn't match
    if ( first_entry._magic != RPK_FILE_MAGIC ) goto ERR;


    // sanity check; header size should be greater than zero, and evenly divisible by the size of a header
    // entry.
    if ( ( first_entry._header_length == 0 ) ||
         ( ( first_entry._header_length % sizeof ( rpk_header_entry ) ) != 0 ) ) goto ERR;
     
    rpk_archive *result = malloc(sizeof (rpk_archive) + first_entry._header_length);

    // OOM
    if ( result == NULL ) goto ERR;


    *result = (rpk_archive) {
        .source = source,
        .entry_count = first_entry._header_length / sizeof (rpk_header_entry)
    };

    //reset the file pointer so we just re-read the first record
    if ( fseek(source,0,SEEK_SET) )
        goto POST_ALLOC_ERR;

    // read the header into the archive struct, and abort of we read the wrong number of entries.
    if ( fread(result->header, sizeof (rpk_header_entry), result->entry_count, source) != result->entry_count )
        goto POST_ALLOC_ERR;

    return result;

POST_ALLOC_ERR:
    free(result);
ERR:
    fclose(source);
    return NULL;
}


// expand archive entries to be files on disk. resulting files will have .rent file extension
int rpk_extract(rpk_archive* archive, const char* dest_dir) {
    int status = 0;
    char pwd[PATH_MAX];

    if (dest_dir != NULL) {
        getcwd(pwd, PATH_MAX);

        if ( chdir(dest_dir) ) {
            status = RPK_EXERR_CDDEST;
            goto CLEANUP;
        };
    }
    
    for ( size_t i = 0; i < archive->entry_count; i++ ) {

        // construct an appropriate filename for the entry
        char namebuf[RPK_ENT_FNAME_SIZ];

        memset(namebuf,0,RPK_ENT_FNAME_SIZ);
        strncpy(namebuf, archive->header[i].name, RPK_ENT_NAME_SIZ);
        strncat(namebuf, RPK_ENT_FNAME_EXT, RPK_ENT_FNAME_EXT_SIZ + 1);

        // try to open output file for writing
        FILE *output = fopen(namebuf,"wb");

        if ( output == NULL ) {
            status = RPK_EXERR_WRITEENT;
            goto CLEANUP;
        }

        // abort if we cannot seek to the start of the entry payload in the file
        if ( fseek(archive->source, archive->header[i].offset, SEEK_SET) ) {
            status = RPK_EXERR_READARCH;
            goto CLEANUP;
        }

        // how many read/write operations are required to write the entry to disk based on buffer size
        uint32_t opcount = archive->header[i].length / BUFSIZ;

        // if header length isn't evenly divisible by bufsiz, we need one more operation to write the remaining
        // data
        if ( ( archive->header[i].length % BUFSIZ ) > 0 ) opcount++;


        for ( int i = 0; i < opcount; i++ ) {

            char *readbuf[BUFSIZ];
            int last_read = fread(readbuf, 1, BUFSIZ, archive->source);
            
            if ( last_read <= 0 ) {
                status = RPK_EXERR_READARCH;
                goto CLEANUP;
            }

            // probably not necessary, but flush output so we always have the entire
            // buffer to work with
            fflush(output);
            int last_write = fwrite(readbuf, 1, last_read, output);

            // if we were unable to write as much data as we read then we give up
            if ( last_write != last_read ) {
                status = RPK_EXERR_WRITEENT;
                goto CLEANUP;
            }
        }
    }

CLEANUP:
    if (dest_dir != NULL) chdir(pwd);
    return status;

}

// teardown archive structure and free associated memory
void rpk_unload(rpk_archive* archive) {
    fclose(archive->source);
    archive->source = NULL;
    archive->entry_count = 0;
    free(archive);
}
