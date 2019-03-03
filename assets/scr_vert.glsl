#version 330

in vec2 position;
in vec2 texcoord;
out vec2 Texcoord;

void main(void)
{
  Texcoord = texcoord;
  gl_Position = vec4(position, 0.0, 1.0);
}

