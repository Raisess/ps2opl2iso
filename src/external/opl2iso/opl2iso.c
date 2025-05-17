/*
  Copyright 2010, volca
  Parts Copyright 2009, jimmikaelkael
  Copyright (c) 2002, A.Lee & Nicholas Van Veen
  Licenced under Academic Free License version 3.0
  Review OpenUsbLd README & LICENSE files for further details.

  Some parts of the code are taken from libcdvd by A.Lee & Nicholas Van Veen
  Review license_libcdvd file for further details.
*/

#include "opl2iso.h"

u32 crctab[0x400];

#define EXIT_OK      0
#define EXIT_FAILURE 1

char* __concat(const char *s1, const char *s2)
{
    const size_t len1 = strlen(s1);
    const size_t len2 = strlen(s2);
    char *result = malloc(len1 + len2 + 1);
    memcpy(result, s1, len1);
    memcpy(result + len1, s2, len2 + 1);
    return result;
}

char* concat_path(const char *s1, const char *s2)
{
  return __concat(__concat(s1, "/"), s2);
}

char spin(int i)
{

    switch (i % 4) {
        case 0:
            return '|';
        case 1:
            return '/';
        case 2:
            return '-';
        default:
        case 3:
            return '\\';
    }
}

u32 crc32(const char *string)
{
    int crc, table, count, byte;

    for (table = 0; table < 256; table++) {
        crc = table << 24;

        for (count = 8; count > 0; count--) {
            if (crc < 0)
                crc = crc << 1;
            else
                crc = (crc << 1) ^ 0x04C11DB7;
        }
        crctab[255 - table] = crc;
    }

    do {
        byte = string[count++];
        crc = crctab[byte ^ ((crc >> 24) & 0xFF)] ^ ((crc << 8) & 0xFFFFFF00);
    } while (string[count - 1] != 0);

    return crc;
}

void compute_name(char *buffer, int maxlen, const char *game_name, const char *game_id, int partid)
{
    if (strlen(game_id) <= 3) {
        buffer[0] = '\0';
        return;
    }

    snprintf(buffer, maxlen, "ul.%08X.%s.%02d", crc32(game_name), &game_id[3], partid);
}

int find_game(const char *path, const char *gameid, cfg_t *item)
{
    FILE *ul;

    if (!item) {
        printf("Invalid target record (internal error)!\n");
        return EXIT_FAILURE;
    }

    // open ul.cfg
    ul = fopen(concat_path(path, "ul.cfg"), "rb");
    if (ul == NULL) {
        printf("No ul.cfg in the current directory!\n");
        return EXIT_FAILURE;
    }

    fseek(ul, 0, SEEK_END);
    size_t fsize = ftell(ul);
    fseek(ul, 0, SEEK_SET);

    size_t items = fsize / 64;

    while (items-- > 0) {
        memset(item->name, 0, 33);
        memset(item->image, 0, 16);

        fread(item->name, 1, 32, ul);
        fread(item->image, 1, 15, ul);
        fread(&item->parts, 1, sizeof(item->parts), ul);
        fread(&item->media, 1, sizeof(item->media), ul);
        fread(item->pad, 1, sizeof(item->pad), ul);

        // in the ul.????_???.?? form?
        if (strncmp(item->image, gameid, 15) == 0) {
            fclose(ul);
            return 1;
        }

        // in the ????_???.?? form?
        if (strncmp(&item->image[3], gameid, 12) == 0) {
            fclose(ul);
            return 1;
        }

        // game name itself?
        if (strncmp(item->name, gameid, 32) == 0) {
            fclose(ul);
            return 1;
        }

        // in the ul.????????.????_???.??.?? form? (Drag-Dropped part-file name)
        char *temp;
#ifdef _WIN32
        temp = strrchr(gameid, '\\');
#else
        temp = strrchr(gameid, '/');
#endif
        if (temp == NULL)
            temp = (char *)gameid;
        else
            temp++;
        if ((strlen(temp) >= 23)                                // Minimum part file name length is really 26
            && (strncmp("ul.", temp, 3) == 0)                   //"ul." at start
            && (strncmp(&item->image[2], &(temp[11]), 12) == 0) //.game_ID after CRC
        ) {
            fclose(ul);
            return 1;
        }
    }

    fclose(ul);
    return EXIT_OK;
}

int export_game(const char *src_path, const char *dest_path, const char *gameid)
{
    cfg_t grecord;

    // first try to load the game record with specified code
    if (!find_game(src_path, gameid, &grecord)) {
        printf("Game not found in the ul.cfg!\n");
        return EXIT_FAILURE;
    }

    char rname[255];
    const char *mediatype;

    // CD: 0x12, DVD: 0x14

    if (grecord.media == 0x12) {
        mediatype = "CD";
    } else if (grecord.media == 0x14) {
        mediatype = "DVD";
    } else {
        printf("Invalid media ID!\n");
        return EXIT_FAILURE;
    }

    if (strlen(grecord.image) <= 3) {
        printf("Invalid game record image name\n");
        return EXIT_FAILURE;
    }

    char temp_ID[12];
    strncpy(temp_ID, grecord.image + 3, 11);
    temp_ID[11] = 0;

#ifdef _WIN32
    snprintf(rname, 255, "%s\\%s.%s.iso", mediatype, temp_ID, grecord.name);
#else
    snprintf(rname, 255, "%s/%s.%s.iso", mediatype, temp_ID, grecord.name);
#endif

    printf("Game found in the ul.cfg. Resulting file name: '%s'\n", rname);

    FILE *fsrc = NULL;

    const char *fullrname = concat_path(dest_path, rname);
    FILE *fdest = fopen(fullrname, "wb");

    if (!fdest) {
        printf("Could not open destination file '%s', exiting\n", fullrname);
        return EXIT_FAILURE;
    }

    int part;
    for (part = 0; part < grecord.parts; part++) {
        char sname[128];

        compute_name(sname, 128, grecord.name, grecord.image, part);

        printf("Processing part %d/%d: '%s'...\n", part + 1, grecord.parts, sname);

        fsrc = fopen(concat_path(src_path, sname), "rb");

        if (!fsrc) {
            fclose(fdest);
            printf("Could not open source file '%s', resulting file will be invalid\n", sname);
            return EXIT_FAILURE;
        }

        unsigned char block[65536];
        int i = 0;

        while (!feof(fsrc)) {
            printf("\r%c\r", spin(i++));
            size_t read = fread(block, 1, 65536, fsrc);
            if (read != fwrite(block, 1, read, fdest)) {
                printf("Could not write to destination file '%s' (Insufficient disk space?). Resulting file will be invalid\n", rname);
                fclose(fdest);
                fclose(fsrc);
                return EXIT_FAILURE;
            }
        }
    }

    fclose(fdest);
    fclose(fsrc);

    printf("* All parts processed...\n");

    return EXIT_OK;
}
