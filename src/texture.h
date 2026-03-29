#ifndef TEXTURE_H
#define TEXTURE_H

#include "primitives.h"

#include "sokol_gfx.h"

#include "texture_kind.h"
#define X_TEXTURE_KINDS(name, ...) name,
typedef enum {
	TEXTURE_KINDS
} TextureKind;

#include "texture_kind.h"
#define X_TEXTURE_KINDS(...) +1
enum { TextureKindCount = TEXTURE_KINDS };

StringView str_from_texture_kind(TextureKind kind);
TextureKind texture_kind_from_str(StringView *str);

void checkerboard(sg_image *img);
void solid_white(sg_image *img);
void solid_black(sg_image *img);

#endif // TEXTURE_H
