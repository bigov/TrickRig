#version 330

// входные параметры
in vec2 vFragment; // 2D коодината в текстурной карте
in vec4 vColor;    // цвет
in vec4 vDiff;     // диффузный свет
flat in int vX;   // индекс вершины для определения выделенного фрагмента
flat in int vY;   // индекс вершины для определения выделенного фрагмента
flat in int vZ;   // индекс вершины для определения выделенного фрагмента

uniform sampler2D texture_0;

//layout(location = 0) out vec4 FragColor;
//layout(location = 1) out uvec3 FragData;

out vec4 FragColor;
out uvec3 FragData;

void main(void)
{
  //vec2 flip_fragment = vec2(vFragment.x, 1.0f - vFragment.y);
  //FragColor = vColor * vDiff + texture(texture_0, flip_fragment);
  FragColor = vColor * vDiff + texture(texture_0, vFragment);

  FragColor.a = vColor.a;
  if(!gl_FrontFacing) FragColor =
      vec4(FragColor.r * 0.25, FragColor.g * 0.25, FragColor.b * 0.25, FragColor.a);

  //FragData = uvec3(vId, gl_PrimitiveID, 4222111000);
  FragData = uvec3(vX, vY, vZ);

  // Индексы примитивов пишем во второй буфер
  //int r = 0;
  //int g = 0;
  //int b = 0;
  //b = vId;
  //r  = b % 255;
  //b /= 255;
  //g  = b % 255;
  //b /= 255;
  //r = (vId & 0x000000FF) >>  0;
  //g = (vId & 0x0000FF00) >>  8;
  //b = (vId & 0x00FF0000) >> 16;
  //FragData = vec4 (r/255.0f, g/255.0f, b/255.0f, 1.0f);
  //FragColor.r = FragData.r;
  //FragColor.g = FragData.g;
  //FragColor.b = FragData.b;
}
