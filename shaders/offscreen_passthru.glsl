@header #include "handmade_math.h"
@header #include "sokol_gfx.h"

@ctype vec2 HMM_Vec2

@vs offscreen_passthru_vs
out vec2 uv; // texcoords are in the normalized [0,1] range for the viewport-filling quad part of the triangle

void
main(
	void
) {
	vec2 vertices[3] = vec2[](
		vec2(-1, -1),
		vec2( 3, -1),
		vec2(-1,  3)
	);
	gl_Position = vec4(vertices[gl_VertexIndex], 0., 1.);
	uv = 0.5 * gl_Position.xy + vec2(0.5);
}
@end

@fs offscreen_passthru_fs
layout(binding=0) uniform sampler texSampler;
layout(binding=0) uniform texture2D offscreenTexture;
layout(binding=0) uniform postprocess_params {
	vec2 screen_dims;
};
in vec2 uv;
out vec4 FragColor;

void
main(
	void
) {
	vec2 offset = vec2(1.f) / screen_dims;
	vec2 TexCoords = vec2(uv.x, 1.f - uv.y);

	// vec3 img_color = vec3(texture(sampler2D(offscreenTexture , texSampler), TexCoords));

	// invert image
	// FragColor = vec4(vec3(1. - img_color), 1.);

	// convert image to grayscale
	// float average = (img_color.r + img_color.g + img_color.b) / 3.;
	// FragColor = vec4(average, average, average, 1.f);

	// Kernel-based post-processing
	vec2 offsets[9] = vec2[](
		vec2(-offset.x,  offset.y), // top-left
		vec2(    0.   ,  offset.y), // top-center
		vec2( offset.x,  offset.y), // top-right
		vec2(-offset.x,     0.   ), // center-left
		vec2(    0.   ,     0.   ), // center-center
		vec2( offset.x,     0.   ), // center-right
		vec2(-offset.x, -offset.y), // bottom-left
		vec2(    0.   , -offset.y), // bottom-center
		vec2( offset.x, -offset.y)   // bottom-right   
	);

	/* SHARPEN */
	// float kernel[9] = float[](
	// 	-1, -1, -1,
	// 	-1,  9, -1,
	// 	-1, -1, -1
	// );

	/* BLUR */
	// float kernel[9] = float[](
	// 	1. / 16, 2. / 16, 1. / 16,
	// 	2. / 16, 4. / 16, 2. / 16,
	// 	1. / 16, 2. / 16, 1. / 16
	// );

	/* EDGE DETECTION */
	float kernel[9] = float[](
		1,  1, 1,
		1, -8, 1,
		1,  1, 1
	);

	vec3 sampleTex[9];
	for (int i = 0; i < 9; ++i) {
		sampleTex[i] = vec3(texture(sampler2D(offscreenTexture , texSampler), TexCoords + offsets[i]));
	}
	vec3 col = vec3(0.);
	for (int i = 0; i < 9; ++i) {
		col += sampleTex[i] * kernel[i];
	}
	FragColor = vec4(col, 1.);

	// convert image to grayscale (weighted)
	float average = 0.2126 * FragColor.r + 0.7152 * FragColor.g + 0.0722  * FragColor.b;
	FragColor = vec4(average, average, average, 1.f);

}
@end

@program offscreen_passthru offscreen_passthru_vs offscreen_passthru_fs

