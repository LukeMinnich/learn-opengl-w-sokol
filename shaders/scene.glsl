@header #include "sokol_gfx.h"

@vs scene_vs
out vec2 tex_coords;

const vec2 super_triangle[3] = vec2[](
	vec2(-1, -1),
	vec2( 3, -1),
	vec2(-1,  3)
);

void main(
	void
) {
	gl_Position = vec4(super_triangle[gl_VertexIndex], 0, 1);
	tex_coords = 0.5 * gl_Position.xy + vec2(0.5);
}
@end

@fs scene_fs
layout(binding=0) uniform scene_fs_params {
	vec2 iResolution;
	float iTime;
};

out vec4 fragColor;
in vec4 gl_FragCoord;
in vec2 tex_coords;

float sdSphere( in vec2 p, in float r )
{
    p = abs(p);
    p = max(p,p.yx-r);
    float a = 0.5*(p.x+p.y);
    float b = 0.5*(p.x-p.y);
    return a - sqrt(r*r*0.5-b*b);
}

vec4 sdgSphere( in vec3 p, in float r )
{
    float l = length(p);
    return vec4(l-r, p/l);
}

#define AA 3

void main(
	void
) {
	/*
	vec2 frag_coord = gl_FragCoord.xy / iResolution;
	frag_coord.y = (1.f - frag_coord.y);
	fragColor = vec4(0.f, frag_coord.x, frag_coord.y, 1.f);
	fragColor = vec4(0.f, tex_coords.x, tex_coords.y, 1.f);
	fragColor = vec4(0.f, tex_coords.x, tex_coords.y, 1.f);
	*/

	/*
	vec2 p = (2.0 * gl_FragCoord.xy - iResolution.xy) / iResolution.y;
	*/

	// /*
	vec2 fragCoord = gl_FragCoord.xy;
	vec2 p = (2.0*(fragCoord)-iResolution.xy) / min(iResolution.y, iResolution.x);

	const float rad = 0.5;
	float d = sdSphere(p, rad);

	vec3 col = vec3(1.0,0.9,1.0) + sign(d)*vec3(-0.3,0.4,0.3);
	col *= 1.0 - exp(-3.0*abs(d));
	col *= 0.8 + 0.2*cos(150.0*d);
	col = mix( col, vec3(1.0), 1.0-smoothstep(0.0,0.008,abs(d)) );

	fragColor = vec4(col, 1.);
	// */

	/*
	vec2 fragCoord = gl_FragCoord.xy;

	// camera movement
	float an = 0.5*(iTime-10.0);
	vec3 ro = 1.4*vec3( 1.0*cos(an), 0.5, 1.0*sin(an) );
	vec3 ta = vec3( 0.0, 0.0, 0.0 );

	// camera matrix
	vec3 ww = normalize( ta - ro );
	vec3 uu = normalize( cross(ww,vec3(0.0,1.0,0.0) ) );
	vec3 vv = normalize( cross(uu,ww));

	// animate sphere
	float ra = 0.5 + 0.2 * sin(iTime);

	// render
	vec3 tot = vec3(0.0);
	for (int m = 0; m < AA; m++) {
		for (int n = 0; n < AA; n++) {
			// pixel coordinates
			vec2 o = vec2(float(m),float(n)) / float(AA) - 0.5;
			vec2 p = (2.0*(fragCoord+o)-iResolution.xy)/iResolution.y;

			// create view ray
			vec3 rd = normalize( p.x*uu + p.y*vv + 2.0*ww );

			// raymarch
			const float t_max = 5.;
			const float h_min = 0.0001;

			float t = 0.;
			for (int i = 0; i < 256; i++) {
				vec3 pos = ro + t * rd;
				float h = sdgSphere(pos, ra).x;
				// h = max(h, pos.z); // use to see interior gradient
				if(   h < h_min
				   || t > t_max ) {
					break;
				}
				t += h;
			}

			// shading/lighting
			vec3 col = vec3(0.0);
			if (t < t_max) {
				vec3 pos = ro + t * rd;
				vec3 nor = sdgSphere(pos,ra).yzw;

				// numerical normal for comparison (https://iquilezles.org/articles/normalsSDF)
				#if 0
				const vec2 e = vec2(1,-1);
				const float eps = 0.0002;
				nor = normalize(  e.xyy*sdgSphere( pos + e.xyy*eps, ra ).x
				                + e.yyx*sdgSphere( pos + e.yyx*eps, ra ).x
				                + e.yxy*sdgSphere( pos + e.yxy*eps, ra ).x
				                + e.xxx*sdgSphere( pos + e.xxx*eps, ra ).x );
				#endif

				float dif = clamp( dot(nor,vec3(0.57703)), 0.0, 1.0 );
				float amb = 0.5 + 0.5*dot(nor,vec3(0.0,1.0,0.0));
				col = vec3(0.2,0.3,0.4)*amb + vec3(0.85,0.75,0.65)*dif;
				col *= (0.5+0.5*nor)*(0.5+0.5*nor);
			}

			// gamma (yes, before accumulation even though light accum is linear)
			col = sqrt(col);
			tot += col;
		}
	}

	tot /= float(AA*AA);

	fragColor = vec4(tot, 1.0);
	*/
}
@end

@program scene scene_vs scene_fs

