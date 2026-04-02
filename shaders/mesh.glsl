@header #include "handmade_math.h"
@header #include "sokol_gfx.h"

@ctype mat4 HMM_Mat4
@ctype vec3 HMM_Vec3
@ctype vec2 HMM_Vec2

//=== shadow pass
@vs vs_shadow
@glsl_options fixup_clipspace // important: map clipspace z from -1..+1 to 0..+1 on GL

layout(binding=0) uniform vs_shadow_params {
    mat4 vp;
    mat4 model;
};

in vec3 position;
in vec3 iposition;

void main() {
    gl_Position = vp * (model * vec4(position, 1.) + vec4(iposition, 0.f));
}
@end

@fs fs_shadow
void main() { }
@end

@program shadow vs_shadow fs_shadow

//=== display pass
@vs vs_display

layout(binding=0) uniform vs_display_params {
	mat4 vp;
	mat4 model;
	mat4 normal_matrix;
	mat4 light_vp;
};

in vec3 position;
in vec3 normal;
in vec3 tangent;
in vec2 tex_coord;
in vec3 iposition;

out vec4 light_proj_pos;

out THRU {
	vec3 world_pos;
	vec3 world_norm;
	vec2 tex_coord;
} vs_out;

void
main(
	void
) {
	/* NOTE(luke): instance position is really a direction vector (an offset position), hence its
	   `w` ordinate is zero. */
	vec4 world_pos = model * vec4(position, 1.) + vec4(iposition, 0.);
	gl_Position = vp * world_pos;
	light_proj_pos = light_vp * world_pos;
	#if !SOKOL_GLSL
		light_proj_pos.y = -light_proj_pos.y;
	#endif
	vs_out.world_pos = vec3(world_pos);
	vs_out.world_norm = mat3(normal_matrix) * normal;
	vs_out.tex_coord = tex_coord;
}
@end

@fs fs_display
layout(binding = 0) uniform sampler tex_sampler;
layout(binding = 1) uniform fs_display_params {
	vec3 light_dir;
	vec3 eye_pos;
	int n_dir_lights;
	int n_point_lights;
};

layout(binding = 0) uniform texture2D diffuseTexture;
layout(binding = 1) uniform texture2D specularTexture;

struct DirLight {
	vec3 direction;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};
layout(binding = 2) readonly buffer in_dir_lights {
	DirLight dir_lights[];
};

struct PointLight {
	vec3 position;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float constant;
	float linear;
	float quadratic;
};
layout(binding=3) readonly buffer in_point_lights {
	PointLight point_lights[];
};

layout(binding = 10) uniform texture2D shadow_map;
layout(binding = 10) uniform sampler shadow_sampler;

in vec4 light_proj_pos;

in THRU {
	vec3 world_pos;
	vec3 world_norm;
	vec2 tex_coord;
} fs_in;

out vec4 frag_color;

void
apply_gamma_correction(
	inout vec3 color
) {
	color = pow(abs(color), vec3(1. / 2.2));
}

void
srgb_to_linear(
	inout vec3 color
) {
	color = pow(abs(color), vec3(2.2));
}

float
pcf_shadow(
	vec4 light_proj_pos_
) {
	vec3 light_pos = light_proj_pos_.xyz / light_proj_pos_.w;
	vec3 sm_pos = vec3((light_pos.xy + 1.0) * 0.5, light_pos.z);
	if (sm_pos.z < 0. || sm_pos.z > 1.) {
		/* Fragment lands outside of shadow map orthographic view frustum. */ 
		return 0.f; // corresponds to SG_BORDERCOLOR_OPAQUE_BLACK
	}
	/* NOTE(luke): it is my understanding that `sampler2DShadow()` usually performs some sort of PCF,
	   though it may be implementation defined. */
	return texture(sampler2DShadow(shadow_map, shadow_sampler), sm_pos);
}

vec3
calc_dir_light(
	DirLight light,
	vec3 normal,
	vec3 view_dir
) {
	vec3 lightDir = normalize(-light.direction);
	// sample texture for diffuse/ambient
	vec3 sampled_diffuse  = vec3(texture(sampler2D(diffuseTexture , tex_sampler), fs_in.tex_coord));
	vec3 sampled_specular = vec3(texture(sampler2D(specularTexture, tex_sampler), fs_in.tex_coord));
	srgb_to_linear(sampled_diffuse);
	// ambient
	vec3 ambient = light.ambient * sampled_diffuse;
	// diffuse
	float diff = max(dot(normal, lightDir), 0.);
	vec3 diffuse = light.diffuse * diff * sampled_diffuse;
	// specular (w/ Blinn-Phong modifications)
	vec3 halfwayDir = normalize(lightDir + view_dir);
	float mat_shininess = 128.;
	float spec = pow(max(dot(normal, halfwayDir), 0.), mat_shininess);
	vec3 specular = light.specular * spec * sampled_specular;
	// shadow
	float shadow = pcf_shadow(light_proj_pos);
	return ambient + shadow * (diffuse + specular);
}

vec3
calc_point_light(
	PointLight light,
	vec3 normal,
	vec3 viewDir,
	vec3 fragPos
) {
	DirLight equiv_dir_light;
	equiv_dir_light.direction = fragPos - light.position, // expect normalization in `calc_dir_light()`
	equiv_dir_light.ambient   = light.ambient;
	equiv_dir_light.diffuse   = light.diffuse;
	equiv_dir_light.specular  = light.specular;

	vec3 result = calc_dir_light(equiv_dir_light, normal, viewDir);

	// attenuation
	float distance = length(light.position - fragPos);
	float attenuation = 1. / (light.constant + light.linear * distance + light.quadratic * distance * distance);

	return attenuation * result;
}

void
main(
	void
) {
	vec3 normal = normalize(fs_in.world_norm);
	vec3 view_dir = normalize(eye_pos - fs_in.world_pos);

	vec3 result = vec3(0.);
	for (int i = 0; i < n_dir_lights; ++i) {
		result += calc_dir_light(dir_lights[i], normal, view_dir);
	}
	for (int i = 0; i < n_point_lights; ++i) {
		result += calc_point_light(point_lights[i], normal, view_dir, fs_in.world_pos);
	}

	apply_gamma_correction(result);
	frag_color = vec4(result, 1.);
}
@end

@program display vs_display fs_display

//=== debug visualization sampler to render shadow map as regular texture
@vs vs_dbg
@glsl_options flip_vert_y

in vec2 pos;
out vec2 uv;

void main() {
    gl_Position = vec4(pos*2.0 - 1.0, 0.5, 1.0);
    uv = pos;
    #if !SOKOL_GLSL
        uv.y = 1.f - uv.y;
    #endif
}
@end

@fs fs_dbg
@image_sample_type dbg_tex unfilterable_float
@sampler_type dbg_smp nonfiltering
layout(binding=0) uniform texture2D dbg_tex;
layout(binding=0) uniform sampler dbg_smp;

in vec2 uv;
out vec4 frag_color;

void main() {
    frag_color = vec4(texture(sampler2D(dbg_tex, dbg_smp), uv).xxx, 1.0);
}
@end

@program dbg vs_dbg fs_dbg
