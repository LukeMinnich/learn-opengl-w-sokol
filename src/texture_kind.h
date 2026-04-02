#ifndef X_TEXTURE_KINDS
#define X_TEXTURE_KINDS(...)
#endif

#ifndef TEXTURE_KINDS
#define TEXTURE_KINDS                                           \
  X_TEXTURE_KINDS(TextureKindNone    , "none"    , solid_white) \
  X_TEXTURE_KINDS(TextureKindDiffuse , "diffuse" , solid_white) \
  X_TEXTURE_KINDS(TextureKindSpecular, "specular", solid_gray ) \
  X_TEXTURE_KINDS(TextureKindEmissive, "emissive", solid_black) \
  X_TEXTURE_KINDS(TextureKindNormal  , "normal"  , solid_norm ) \
  X_TEXTURE_KINDS(TextureKindParallax, "parallax", solid_black) \
  NOOP
#endif

#undef X_TEXTURE_KINDS
