#ifndef IMAGE_H
#define IMAGE_H

#include "sokol_gfx.h"
#include <stdbool.h>

void load_image(const char *filename, bool flip, sg_image *img);

#endif // IMAGE_H
