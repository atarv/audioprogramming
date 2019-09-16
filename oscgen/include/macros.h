#pragma once

#define ON_FOPEN_ERROR(file, filename)                                         \
    {                                                                          \
        if (freq_file == NULL)                                                 \
        {                                                                      \
            printf("Error: unable to read %s\n", argv[ARG_FREQ_BRKFILE]);      \
            error++;                                                           \
            goto cleanup;                                                      \
        }                                                                      \
    }

#define ON_MALLOC_ERROR(pointer)                                               \
    {                                                                          \
        if (pointer == NULL)                                                   \
        {                                                                      \
            printf("No memory\n");                                             \
            error++;                                                           \
            goto cleanup;                                                      \
        }                                                                      \
    }
