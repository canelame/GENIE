#version 330 core
in vec2 TexCoords;
out vec4 color;

uniform sampler2D u_pp_texture;

void main()
{ 
    color = texture(u_pp_texture, TexCoords);
}