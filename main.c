#include <stdio.h>
#include "rpack.h"

int main(int argc, char** argv) {

    if (argc != 2) {
        printf("usage: rpk_extract.exe <archive.rpk>\n");
    }

    rpk_archive *archive;

    if ( (archive = rpk_load(argv[1])) == NULL ) {
        perror("Unable to load rpk");
        return -1;
    }

    int extract_status = rpk_extract(archive,NULL);

    switch (extract_status) {
        case RPK_EXERR_CDDEST:
            perror("Unable to switch to output directory");
            goto CLEANUP;

        case RPK_EXERR_READARCH:
            perror("Error while reading archive file");
            goto CLEANUP;

        case RPK_EXERR_WRITEENT:
            perror("Error while writing extracted file");
            goto CLEANUP;
    }

    printf("Success!\n");

CLEANUP:
    rpk_unload(archive);
    return extract_status;
}
