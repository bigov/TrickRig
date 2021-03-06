#version 330

// входные параметры
in vec2 vFragment; // 2D коодината в текстурной карте
in vec4 vColor;    // цвет вершины (r,g,b,a)
flat in int vertId;

uniform sampler2D texture_0;  // координаты текстуры поверхности
//uniform float tex_transp;   // прозрачность текстуры

uniform int MinId;
uniform int MaxId;

layout(location = 0) out vec4 FragColor; // рендер сцены
layout(location = 1) out int FragData;   // для выбора объектов по ID первой вершины

void main(void)
{
  //vec2 flip_fragment = vec2(vFragment.x, 1.0f - vFragment.y);

  vec4 texColor = texture(texture_0, vFragment);

  // Можно управлять прозрачностью текстуры при помощи uniform-переменной
  //FragColor.x = (1.0f - tex_transp * (1.0f - texColor.x)) * vColor.x;
  //FragColor.y = (1.0f - tex_transp * (1.0f - texColor.y)) * vColor.y;
  //FragColor.z = (1.0f - tex_transp * (1.0f - texColor.z)) * vColor.z;

  FragColor.x = texColor.x * vColor.x;
  FragColor.y = texColor.y * vColor.y;
  FragColor.z = texColor.z * vColor.z;
  FragColor.a = vColor.a;

  if(!gl_FrontFacing)
    FragColor = vec4(FragColor.r * 0.5f, FragColor.g * 0.5f, FragColor.b * 0.5f, FragColor.a);

  // Подсветка поверхности текущего прямоугольника под курсором (в центре окна)
  if((vertId >= MinId) && (vertId <= MaxId))
    FragColor = vec4(FragColor.r * 1.15f, FragColor.g * 1.15f, FragColor.b * 1.15f, FragColor.a);

  FragData = vertId;
}
