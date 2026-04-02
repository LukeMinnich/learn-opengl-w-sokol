#include "image.h"

#include "primitives.h"

#include "sokol_gfx.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <assert.h>
#include <stdbool.h>

void
load_image(
	const char *filename,
	bool flip,
	sg_image *img
) {
	stbi_set_flip_vertically_on_load(flip);
	int w, h, n_channels;
	byte *img_data = stbi_load(filename, &w, &h, &n_channels, 4);
	if (img_data) {
		*img = sg_make_image(&(sg_image_desc){
			.width = w,
			.height = h,
			.pixel_format = SG_PIXELFORMAT_RGBA8,
			.data.mip_levels[0] = {
				.ptr = img_data,
				.size = (size_t)(w * h * 4),
			},
		});
		stbi_image_free(img_data);
	} else {
		assert(false);
	}
}

