$input a_position, a_normal, a_color0
$output v_pos, v_view, v_normal, v_color0

#include <bgfx_shader.sh>

void main() {
	vec3 pos = a_position;

	vec3 normal = a_normal.xyz*2.0 - 1.0;

	gl_Position = mul(u_modelViewProj, vec4(pos, 1.0) );
	v_pos = gl_Position.xyz/gl_Position.w;
	v_view = mul(u_modelView, vec4(pos, 1.0) ).xyz;

	v_normal = mul(u_modelView, vec4(normal, 0.0) ).xyz;
	v_color0 = a_color0;
}
