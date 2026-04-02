#include "texture.h"
#include "sokol_gfx.h"

#include <assert.h>

#define TEXTURE_PIXEL_FORMAT SG_PIXELFORMAT_RGBA8 

#include "texture_kind.h"
#define X_TEXTURE_KINDS(kind, lbl, ...) [kind] = sv(lbl),
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
	for (TextureKind kind = 0; kind < TextureKindCount; ++kind) {
		if (sv_eq(texture_kind_strings[kind], *str)) {
			return kind;
		}
	}
	return TextureKindNone;
}

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

static sg_image check = {0};
static sg_image white = {0};
static sg_image grayy = {0};
static sg_image black = {0};
static sg_image normy = {0};

#define W_CHECKERBOARD 4
#define H_CHECKERBOARD 4
static const byte checkerboard_data[4 * W_CHECKERBOARD * H_CHECKERBOARD] = {
	0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 
	0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 
};
static const byte solid_white_data[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
static const byte solid_grayy_data[4] = { 0x7F, 0x7F, 0x7F, 0xFF };
static const byte solid_black_data[4] = { 0x00, 0x00, 0x00, 0xFF };
/* NOTE(luke): this isn't exactly perfect on the x and y axes, because 0xFF is not evenly
   divisible by two. This is in linear color space and needs no srgb correction. */
static const byte default_normy_data[4] = { 0x7F, 0x7F, 0xFF, 0xFF };

sg_image
checkerboard(
	void
) {
	if (!check.id) {
		img_from_data(&bv(checkerboard_data), (IVec2){W_CHECKERBOARD, H_CHECKERBOARD}, &check);
	}
	return check;
}

sg_image
solid_white(
	void
) {
	if (!white.id) {
		img_from_data(&bv(solid_white_data), (IVec2){1, 1}, &white);
	}
	return white;
}

sg_image
solid_gray(
	void
) {
	if (!grayy.id) {
		img_from_data(&bv(solid_grayy_data), (IVec2){1, 1}, &grayy);
	}
	return grayy;
}

sg_image
solid_black(
	void
) {
	if (!black.id) {
		img_from_data(&bv(solid_black_data), (IVec2){1, 1}, &black);
	}
	return black;
}

sg_image
solid_norm(
	void
) {
	if (!normy.id) {
		img_from_data(&bv(default_normy_data), (IVec2){1, 1}, &normy);
	}
	return normy;
}

static sg_view check_texture = {0};
static sg_view white_texture = {0};
static sg_view black_texture = {0};
static sg_view grayy_texture = {0};
static sg_view normy_texture = {0};

sg_view
checkerboard_texture(
	TextureKind kind
) {
	if (!check_texture.id) {
		check_texture = sg_make_view(&(sg_view_desc){
			.texture.image = checkerboard(),
			.label = str_from_texture_kind(kind).ptr,
		});
	}
	return check_texture;
}

sg_view
solid_white_texture(
	TextureKind kind
) {
	if (!white_texture.id) {
		white_texture = sg_make_view(&(sg_view_desc){
			.texture.image = solid_white(),
			.label = str_from_texture_kind(kind).ptr,
		});
	}
	return white_texture;
}

sg_view
solid_gray_texture(
	TextureKind kind
) {
	if (!grayy_texture.id) {
		grayy_texture = sg_make_view(&(sg_view_desc){
			.texture.image = solid_gray(),
			.label = str_from_texture_kind(kind).ptr,
		});
	}
	return grayy_texture;
}

sg_view
solid_black_texture(
	TextureKind kind
) {
	if (!black_texture.id) {
		black_texture = sg_make_view(&(sg_view_desc){
			.texture.image = solid_black(),
			.label = str_from_texture_kind(kind).ptr,
		});
	}
	return black_texture;
}

sg_view
solid_norm_texture(
	TextureKind kind
) {
	if (!normy_texture.id) {
		normy_texture = sg_make_view(&(sg_view_desc){
			.texture.image = solid_norm(),
			.label = str_from_texture_kind(kind).ptr,
		});
	}
	return normy_texture;
}

static sg_sampler display_sampler = {0};

sg_sampler
mesh_display_sampler(
	void
) {
	if (!display_sampler.id) {
		display_sampler = sg_make_sampler(&(sg_sampler_desc){
			.min_filter = SG_FILTER_LINEAR,
			.mag_filter = SG_FILTER_LINEAR,
			.wrap_u = SG_WRAP_CLAMP_TO_EDGE,
			.wrap_v = SG_WRAP_CLAMP_TO_EDGE,
			.label = "texture-sampler"
		});
	}
	return display_sampler;
}
