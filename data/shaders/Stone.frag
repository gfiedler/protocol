#version 410

in vec4 Color;

layout( location = 0 ) out vec4 FragColor;

void main()
{
    FragColor = Color;
}
