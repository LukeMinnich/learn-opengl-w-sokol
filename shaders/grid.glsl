@header #include "handmade_math.h"
@header #include "sokol_gfx.h"

@ctype mat4 HMM_Mat4
@ctype vec3 HMM_Vec3

@vs grid_vs
layout(binding=0) uniform grid_vs_params {
	mat4 model;
	mat4 view;
	mat4 projection;
};
in vec3 aPos;
out vec2 uv;

void main(
	void
) {
	gl_Position = projection * view * model * vec4(aPos, 1.);
	uv = (model * vec4(aPos, 1.)).xz; // does this still work ok far away from the origin?
}
@end

@fs grid_fs
layout(binding=1) uniform grid_fs_params {
	vec3 grid_color;
};
in vec2 uv;
out vec4 FragColor;

float
pristineGrid(
	vec2 uv,
	vec2 lineWidth
) {
	/* From https://bgolus.medium.com/the-best-darn-grid-shader-yet-727f9278b9d8 */
	vec2 ddx = dFdx(uv);
	vec2 ddy = dFdy(uv);
	vec2 uvDeriv = vec2(length(vec2(ddx.x, ddy.x)), length(vec2(ddx.y, ddy.y)));
	bvec2 invertLine = bvec2(lineWidth.x > 0.5, lineWidth.y > 0.5);
	vec2 targetWidth = vec2(invertLine.x ? 1.0 - lineWidth.x : lineWidth.x,
	                        invertLine.y ? 1.0 - lineWidth.y : lineWidth.y);
	vec2 drawWidth = clamp(targetWidth, uvDeriv, vec2(0.5));
	vec2 lineAA = uvDeriv * 1.5;
	vec2 gridUV = abs(fract(uv) * 2.0 - 1.0);
	gridUV.x = invertLine.x ? gridUV.x : 1.0 - gridUV.x;
	gridUV.y = invertLine.y ? gridUV.y : 1.0 - gridUV.y;
	vec2 grid2 = smoothstep(drawWidth + lineAA, drawWidth - lineAA, gridUV);
	grid2 *= clamp(targetWidth / drawWidth, 0.0, 1.0);
	grid2 = mix(grid2, targetWidth, clamp(uvDeriv * 2.0 - 1.0, 0.0, 1.0));
	grid2.x = invertLine.x ? 1.0 - grid2.x : grid2.x;
	grid2.y = invertLine.y ? 1.0 - grid2.y : grid2.y;
	return mix(grid2.x, 1.0, grid2.y);
}

void main(
	void
) {
	vec2 lineWidth = vec2(0.1);
	FragColor = vec4(grid_color, 1.) * pristineGrid(uv, lineWidth);
}
@end

@program grid grid_vs grid_fs

