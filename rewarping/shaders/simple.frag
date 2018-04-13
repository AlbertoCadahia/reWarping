#version 420
in highp vec3 gColor;
in highp vec2 gTexCoord;

out highp vec4 fColor;//layout(location = 0)

layout (binding = 0) uniform sampler2D colorTex;

vec3 Ka;//Color texture coef


void main()
{
   // Color Texture
   Ka=texture2D(colorTex,gTexCoord).rgb;

   fColor = vec4(Ka,1.0)*vec4(gColor,1.0);
}
