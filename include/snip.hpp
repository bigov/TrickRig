//=============================================================================
//
// file: snip.hpp
//
// Элементы формирования пространства
//
//=============================================================================

#ifndef __SNIP_HPP__
#define __SNIP_HPP__

#include "vbo.hpp"
#include "main.hpp"

namespace tr
{
  // структура для обращения в тексте программы к индексам данных вершин по названиям
  enum SNIP_DATA_ID {
    COORD_X, COORD_Y, COORD_Z, COORD_W,
    COLOR_R, COLOR_G, COLOR_B, COLOR_A,
    NORM_X, NORM_Y, NORM_Z, NORM_W,
    FRAGM_U, FRAGM_V, ROW_STRIDE
  };

  //## Набор данных для формирования в GPU многоугольника с индексацией вершин
  struct snip
  {
    GLsizeiptr data_offset = 0; // положение блока данных в буфере GPU

    // Блок данных для передачи в буфер GPU
    // TODO: более универсальным тут было-бы (?) использование контейнера std::vector<GLfloat>,
    GLfloat data[tr::digits_per_snip] = {
   /*|----3D коодината-------|-------RGBA цвет-------|-----вектор нормали------|--текстура----|*/
      0.0f, 0.0f, 0.0f, 1.0f, 0.3f, 0.3f, 0.3f, 1.0f, -.46f, .76f, -.46f, 0.0f, 0.0f,   0.375f,
      0.0f, 0.0f, 1.0f, 1.0f, 0.3f, 0.3f, 0.3f, 1.0f, -.46f, .76f,  .46f, 0.0f, 0.125f, 0.375f,
      1.0f, 0.0f, 1.0f, 1.0f, 0.3f, 0.3f, 0.3f, 1.0f,  .46f, .76f,  .46f, 0.0f, 0.125f, 0.500f,
      1.0f, 0.0f, 0.0f, 1.0f, 0.3f, 0.3f, 0.3f, 1.0f,  .46f, .76f, -.46f, 0.0f, 0.0f,   0.500f,
    };  // нормализованый вектор с небольшим наклоном: .46f, .76f,  .46f

    snip(void) {}
    snip(const snip &);                  // дублирующий конструктор

    snip& operator=(const snip &);       // оператор присваивания
    void copy_data(const snip &);        // копирование данных из другого снипа
    void texture_set(GLfloat, GLfloat);  // установка фрагмента текстуры
    void point_set(float, float, float); // установка 3D координат поверхности

    // Функции управления данными снипа в буферах VBO данных и VBO индекса
    void vbo_append(tr::vbo &);
    bool vbo_update(tr::vbo &, GLsizeiptr offset);
    void vbo_jam   (tr::vbo &, GLintptr dst);
  };

} //namespace tr

#endif
