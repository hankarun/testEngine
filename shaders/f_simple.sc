$input v_pos, v_view, v_normal, v_color0


#include <bgfx_shader.sh>
#include "shaderlib.sh"

vec2 blinn(vec3 _lightDir, vec3 _normal, vec3 _viewDir)
{
	float ndotl = dot(_normal, _lightDir);
	vec3 reflected = _lightDir - 2.0*ndotl*_normal; // reflect(_lightDir, _normal);
	float rdotv = dot(reflected, _viewDir);
	return vec2(ndotl, rdotv);
}

void main() {
	vec2 viewport = (u_viewRect.zw - u_viewRect.xy) * vec2(1.0/8.0, 1.0/4.0);
	vec2 stippleUV = viewport*(v_pos.xy*0.5 + 0.5);

	vec4 color = v_color0;

    gl_FragColor = v_color0;
	vec3 lightDir = vec3(0.0, -1.0, 0.0);
	vec3 normal = normalize(v_normal);
	vec3 view = normalize(v_view);
	vec2 bln = blinn(lightDir, normal, view);
	float l = saturate(bln.y) + 0.12;

	color.xyz = toLinear(color.xyz)*l;
	gl_FragColor = toGamma(color);
}
