#include "texture.h"

#include <assert.h>

#define TEXTURE_PIXEL_FORMAT SG_PIXELFORMAT_RGBA8 

#include "texture_kind.h"
#define X_TEXTURE_KINDS(name, description) [name] = sv(description),
const static StringView texture_kind_strings[TextureKindCount] = { TEXTURE_KINDS };

StringView
str_from_texture_kind(
	TextureKind kind
) {
	return texture_kind_strings[kind];
}

TextureKind
texture_kind_from_str(
	StringView *str
) {
	for (TextureKind i = 0; i < TextureKindCount; ++i) {
		if (sv_eq(texture_kind_strings[i], *str)) {
			return i;
		}
	}
	return TextureKindNone;
}

#define W_CHECKERBOARD 4
#define H_CHECKERBOARD 4
static const byte checkerboard_data[4 * W_CHECKERBOARD * H_CHECKERBOARD] = {
	0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 
	0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 
};

void
img_from_data(
	ByteView *bytes,
	IVec2 dims,
	sg_image *img
) {
	assert(!img->id);
	*img = sg_make_image(&(sg_image_desc){
		.width = (int)dims.x,
		.height = (int)dims.y,
		.pixel_format = TEXTURE_PIXEL_FORMAT,
		.data.mip_levels[0] = {
			.ptr = bytes->data,
			.size = bytes->size,
		},
	});
}

void
checkerboard(
	sg_image *img
) {
	img_from_data(&bv(checkerboard_data), (IVec2){W_CHECKERBOARD, H_CHECKERBOARD}, img);
}

static const byte solid_white_data[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
static const byte solid_black_data[4] = { 0x00, 0x00, 0x00, 0x00 };

void
solid_white(
	sg_image *img
) {
	img_from_data(&bv(solid_white_data), (IVec2){1, 1}, img);
}

void
solid_black(
	sg_image *img
) {
	img_from_data(&bv(solid_black_data), (IVec2){1, 1}, img);
}
