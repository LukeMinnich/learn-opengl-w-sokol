#ifndef X_TEXTURE_KINDS
#define X_TEXTURE_KINDS(name)
#endif

#ifndef TEXTURE_KINDS
#define TEXTURE_KINDS                              \
  X_TEXTURE_KINDS(TextureKindNone    , "none"    ) \
  X_TEXTURE_KINDS(TextureKindDiffuse , "diffuse" ) \
  X_TEXTURE_KINDS(TextureKindSpecular, "specular") \
  X_TEXTURE_KINDS(TextureKindEmissive, "emissive") \
  X_TEXTURE_KINDS(TextureKindNormal  , "normal"  ) \
  X_TEXTURE_KINDS(TextureKindParallax, "parallax") \
  NOOP
#endif

#undef X_TEXTURE_KINDS
