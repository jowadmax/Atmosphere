#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "utils.h"
#include "timers.h"
#include "splash_screen.h"
#include "lib/upng.h"
#include "sd_utils.h"
#include "display/video_fb.h"

uint8_t *load_default_splash() {
    printf("Failed to read custom splash. Falling back to default splash");
    uint8_t *buffer = calloc(default_splash_png_size, sizeof(uint8_t));
    memcpy(buffer, default_splash_png, default_splash_png_size);
    return buffer;
}

void display_splash_screen_png(const char *custom_splash_path, uint8_t* fb_addr) {
    /* Cast the framebuffer to a 2-dimensional array, for easy addressing */
    uint8_t (*fb)[3072] = (uint8_t(*)[3072])fb_addr;

    /* PNG file data and PNG handler */
    uint8_t *splash_screen_contents = NULL;
    uint8_t *splash_screen_pixels = NULL;
    int splash_screen_length = 0;
    upng_t* upng;

    /* Stat the splash file to set size information, and see if it's there */
    struct stat info;
    if (!stat(custom_splash_path, &info)) {
        /* Prepare the buffer */
        splash_screen_contents = calloc(info.st_size, sizeof(uint8_t));
        splash_screen_length = info.st_size;

        /* Read the PNG from the SD card */
        if (!read_sd_file(splash_screen_contents, info.st_size, custom_splash_path)) {
            splash_screen_contents = load_default_splash();
            splash_screen_length = default_splash_png_size;
        }
    } else {
        splash_screen_contents = load_default_splash();
        splash_screen_length = default_splash_png_size;
    }

    /* Open and decode the PNG file */
	upng = upng_new_from_bytes(splash_screen_contents, splash_screen_length);
	upng_decode(upng);
	if (upng_get_error(upng) != UPNG_EOK){
        printf("Error: Failed to decode splash screen! (err %u %u)\n", upng_get_error(upng), upng_get_error_line(upng));
        upng_free(upng);
        free(splash_screen_contents);
		return;
	}

    /* Obtain a pixels array (RGBA) */
    splash_screen_pixels = (uint8_t *) upng_get_buffer(upng);
	if (!splash_screen_pixels){
        printf("Error: Failed to get png pixels!\n");
        upng_free(upng);
        free(splash_screen_contents);
		return;
	}

    int count = 0;
    int png_width = upng_get_width(upng);
    int png_height = upng_get_height(upng);

    for (int y = 0; y < png_height; y++) {
        for (int x = png_width; x > 0; x--) {
            /* Fill the framebuffer w/ necessary pixel translation (framebuffer is RGBA, PNG is RGBA) */
            fb[x][y * 4 + 0] = splash_screen_pixels[count + 0];
            fb[x][y * 4 + 1] = splash_screen_pixels[count + 1];
            fb[x][y * 4 + 2] = splash_screen_pixels[count + 2];
            fb[x][y * 4 + 3] = splash_screen_pixels[count + 3];
            count += 4;
        }
    }

    /* Free the splash buffer and png file; we're done with them. */
    upng_free(upng);
    free(splash_screen_contents);

    /* Display the splash screen for three seconds. */
    wait(3000000);
}
