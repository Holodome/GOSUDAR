#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <windows.h>

enum {
    PROGRAM_MODE_START,
    PROGRAM_MODE_END,
};

int main(int argc, char **argv) {
    unsigned mode = PROGRAM_MODE_START;
    const char *filename = "timer.timer";
    for (int i = 0; i < argc; ++i) {
        char *arg = argv[i];
        if (strcmp(arg, "-start") == 0) {
            mode = PROGRAM_MODE_START;
        } else if (strcmp(arg, "-end") == 0) {
            mode = PROGRAM_MODE_END;
        } 
    }

    if (mode == PROGRAM_MODE_START) {
        FILE *file = fopen(filename, "wb");
        assert(file);
        SYSTEMTIME cur_time = {};
        GetSystemTime(&cur_time);
        fwrite(&cur_time, sizeof(cur_time), 1, file);
        fclose(file);
    } else if (mode == PROGRAM_MODE_END) {
        FILE *file = fopen(filename, "rb");
        SYSTEMTIME cur_time = {};
        GetSystemTime(&cur_time);
        SYSTEMTIME old_time;
        fread(&old_time, sizeof(old_time), 1, file);
        double diff = ((double)cur_time.wSecond - (double)old_time.wSecond) + ((double)cur_time.wMilliseconds - (double)old_time.wMilliseconds) / 1000.0;
        printf("Timer: %.3f Seconds elapsed\n", diff);
    } else {
        assert(false);
    }
      
    return 0;
}