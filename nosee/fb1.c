// fb1  --  a first write to framebuffer
// program written by ChatGPT, 2025-02-12
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <stdint.h>

struct Frame {
        uint8_t *ptr;
        int width;
        int height;
};
struct Cursor {
        int x;
        int y;
};

void dot (struct Frame frame, int x, int y, int color) {
    //  int pos = (f(x) * vinfo.xres + x) * (vinfo.bits_per_pixel / 8);
    //    dot (fb_ptr, dwidth, dheight, x, f(x), 128);
    int blue  = (color & 255);
    int green = (color >> 8) & 255;
    int red   = (color >> 16) & 255;
    int alpha = 255;
    int pos = (y * frame.width + x) * 4;
    frame.ptr[pos + 0] = blue;
    frame.ptr[pos + 1] = green;
    frame.ptr[pos + 2] = red;
    frame.ptr[pos + 3] = alpha;
}
void square (struct Frame f, int x1, int y1, int x2, int y2, int color) {
    for (int x = x1; x <= x2; x++) {
        for (int y = y1; y <= y2; y++) {
            dot (f, x, y, color);
        }
    }
}

void background (struct Frame f, int color) {
    square (f, 0, 0, f.width - 1, f.height - 1, color);
}

int f (int x) {
        return x;
}

void cprint (struct Frame f, struct Cursor *c, int chr) {
        int x1 = c->x * 15;
        int x2 = x1 + 15;
        int y1 = c->y * 20;
        int y2 = y1 + 20;
        int color = 0x808080;
        square (f, x1, y1, x2, y2, color);
        c->x = c->x + 1;
}


int main() {
    int fb_fd = open("/dev/fb0", O_RDWR);
    if (fb_fd == -1) {
        perror("Error opening framebuffer device");
        return 1;
    }

    // Get screen information
    struct fb_var_screeninfo vinfo;
    if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo)) {
        perror("Error reading framebuffer information");
        close(fb_fd);
        return 1;
    }

    int screensize = vinfo.yres_virtual * vinfo.xres_virtual * (vinfo.bits_per_pixel / 8);
    
    // Map framebuffer to memory
    uint8_t *fb_ptr = mmap(NULL, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
    if (fb_ptr == MAP_FAILED) {
        perror("Error mapping framebuffer device to memory");
        close(fb_fd);
        return 1;
    }
    printf("virtual yres=%i  xres=%i \n", vinfo.yres_virtual, vinfo.xres_virtual);
    printf("        yres=%i  xres=%i \n", vinfo.yres, vinfo.xres);
    printf("bits_per_pixel=%i \n", vinfo.bits_per_pixel);
    printf("screensize =   %i \n", screensize);

    int dwidth = vinfo.xres;
    int dheight = vinfo.yres;
    uint8_t dp = *fb_ptr;
    // Draw a red gradient
    printf ("Drawing starts...\n");
    for (int y = 0; y < vinfo.yres; y++) {
        for (int x = 0; x < vinfo.xres; x = x + 12) {
            int pixel_offset = (y * vinfo.xres + x) * (vinfo.bits_per_pixel / 8);
            uint8_t red = (uint8_t)((x * 255) / vinfo.xres);
            uint8_t green = 0;
            uint8_t blue = 64;
            uint8_t alpha = 255;

            if (vinfo.bits_per_pixel == 32) {
                fb_ptr[pixel_offset + 0] = blue;
                fb_ptr[pixel_offset + 1] = green;
                fb_ptr[pixel_offset + 2] = red;
                fb_ptr[pixel_offset + 3] = alpha;
            } else if (vinfo.bits_per_pixel == 16) {
                uint16_t color = ((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3);
                ((uint16_t*)fb_ptr)[y * vinfo.xres + x] = color;
            }
        }
        if (y % 200 == 0) {
           printf ("Hello framebuffer!\n");
        }
    }

    struct Frame frame;
    frame.ptr    = fb_ptr;
    frame.width  = vinfo.xres;
    frame.height = vinfo.yres;

    int char_width = 15;
    int ruler1_pos = char_width * 80;
    int ruler2_pos = char_width * 112;
    int darkgray = 0x808080;
    int darksoftblue = 0x08040C;
    background (frame, darksoftblue);
    square (frame, ruler1_pos , 0, ruler1_pos, frame.height - 1, darkgray);
    square (frame, ruler2_pos , 0, ruler2_pos, frame.height - 1, darkgray);
    for (int x = 0; x < 800; x++) {
        dot (frame, x + 112 * char_width, f(x), 0xFFFFFF);
    }
    struct Cursor cursor;
    cursor.x = 48;
    cursor.y = 25;
    cprint (frame, &cursor, 65);
    cprint (frame, &cursor, 65);
    cprint (frame, &cursor, 65);

    printf("Red gradient drawn! \n\nPress Enter to exit...\n");
    getchar(); // Wait for user input

    // Cleanup
    munmap(fb_ptr, screensize);
    close(fb_fd);
    return 0;
}

