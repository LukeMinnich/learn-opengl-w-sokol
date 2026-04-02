#ifndef TEXTURE_H
#define TEXTURE_H

#include "primitives.h"

#include "sokol_gfx.h"

#include "texture_kind.h"
#define X_TEXTURE_KINDS(kind, ...) kind,
typedef enum {
	TEXTURE_KINDS
} TextureKind;

#include "texture_kind.h"
#define X_TEXTURE_KINDS(...) +1
enum { TextureKindCount = TEXTURE_KINDS };

StringView str_from_texture_kind(TextureKind kind);
TextureKind texture_kind_from_str(StringView *str);

sg_image checkerboard(void);
sg_image solid_white(void);
sg_image solid_gray(void);
sg_image solid_black(void);
sg_image solid_norm(void);

sg_view checkerboard_texture(TextureKind kind);
sg_view solid_white_texture(TextureKind kind);
sg_view solid_gray_texture(TextureKind kind);
sg_view solid_black_texture(TextureKind kind);
sg_view solid_norm_texture(TextureKind kind);

sg_sampler mesh_display_sampler(void);

#endif // TEXTURE_H
