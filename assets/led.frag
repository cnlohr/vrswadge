#version AUTOVER
#include "cnovr.glsl"

out vec4 colorOut;

in vec2 texcoords;
in vec3 position;
in vec3 normal;
in vec4 id;


uniform ivec4 ledcolor[10]; //#MAPUNIFORM ledcolor 19

void main()
{
	float dist = (2.0-4.0*length( texcoords.xy - vec2( 0.5, 0.5 ) ));
	ivec4 ledbase = ledcolor[int(id.x)/4];
	int val = ledbase[int(id.x)%4];
	vec3 basecolor = vec3( (val>>16) & 0xff, (val >> 8) & 0xff, (val >> 0 ) & 0xff ) / 255.;
	colorOut = vec4( basecolor, dist ); 
}
