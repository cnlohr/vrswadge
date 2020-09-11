#version AUTOVER
#include "cnovr.glsl"

out vec4 colorOut;

in vec2 texcoords;
in vec3 position;
in vec3 normal;

uniform vec4 ledcolor; //#MAPUNIFORM ledcolor 19

void main()
{
	float dist = (1.-2.*length( texcoords.xy - vec2( 0.5, 0.5 ) ))
	colorOut = vec4( ledcolor * dist ); 
}
