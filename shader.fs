#version 460 core
out vec4 FragColor;
in vec3 ourColor;

void main()
{
    FragColor = vec4(ourColor.r, ourColor.g, ourColor.b, 1.0);
}