/**
 *
 * file: space.hpp
 *
 * Заголовок класса управления виртуальным пространством
 *
 */

#ifndef SPACE_HPP
#define SPACE_HPP

#include "area.hpp"
#include "image.hpp"
#include "framebuf.hpp"
#include "config.hpp"


namespace tr
{
  extern std::unique_ptr<frame_buffer> RenderBuffer; // рендер-буфер окна

  class space_3d
  {
    public:
      space_3d(std::shared_ptr<trgl>& OpenGLContext);
      ~space_3d(void);

      void load(const std::string& map_current);
      void render(void);
      void cursor_event(double x, double y);
      void resize_event(int width, int height);
      void mouse_event(int _button, int _action, int _mods);
      void keyboard_event(int _key, int _scancode, int _action, int _mods);

      std::shared_ptr<glm::vec3> ViewFrom = nullptr;             // 3D координаты точки положения
      float look_dir[2] = {0.0f, 0.0f};  // Направление: азимут (0 - X) и тангаж (0 - горизОнталь, пи/2 - вертикаль)

    private:
      space_3d(void) = delete;

      std::unique_ptr<std::thread> thread_01 = nullptr;

      // Запретить копирование и перенос экземпляров класса
      space_3d(const space_3d&) = delete;
      space_3d& operator=(const space_3d&) = delete;
      space_3d(space_3d&&) = delete;
      space_3d& operator=(space_3d&&) = delete;

      vbo VBO3d      { GL_ARRAY_BUFFER }; // Буфер данных

      std::shared_ptr<trgl>& OGLContext;         // основное окно приложения
      std::unique_ptr<glsl> Program3d = nullptr; // построение 3D пространства

      static const GLsizei uv_data_size = sizeof(float) * 8; // размер блока UV данных (4 вершины по 2 координаты)

      GLuint vao_3d = 0;
      GLuint texture_3d = 0;

      float cursor_dx = 0.f; // Cмещение мыши в активном окне между кадрами
      float cursor_dy = 0.f; // в режиме 3D (режим прицела) при скрытом курсоре.
      double xpos = 0.0;     // позиция указателя относительно левой границы
      double ypos = 0.0;     // позиция указателя относительно верхней границы

      int fb_way = 0;        // 3D движение front/back
      int rl_way = 0;        // -- right/left
      int ud_way = 0;        // -- up/down

      int on_front = 0;      // нажата клавиша вперед
      int on_back  = 0;      // нажата клавиша назад
      int on_right = 0;      // нажата клавиша вправо
      int on_left  = 0;      // нажата клавиша влево
      int on_up    = 0;      // нажата клавиша вверх
      int on_down  = 0;      // нажата клавиша вниз

      int mouse = -1;
      int key = -1;
      int scancode = -1;
      int action = -1;
      int mods = -1;

      const float
        hPi = static_cast<float>(acos(0)), // половина константы "Пи" (90 градусов)
        Pi  = 2 * hPi,                     // константа "Пи"
        dPi = 2 * Pi;                      // двойная "Пи
      const float up_max = hPi - 0.001f;   // Максимальный угол вверх
      const float down_max = -up_max;      // Максимальный угол вниз

      glm::vec3 light_direction {}; // направление освещения
      glm::vec3 light_bright {};    // яркость света

      // Индексы вершин подсвечиваемого вокселя, на который направлен курсор (центр экрана)
      int hl_vertex_id_from = 0;    // индекс начальной вершины
      int hl_vertex_id_end = 0;     // индекс последней вершины

      // Camera control
      float rl=0.f, ud=0.f, fb=0.f; // скорость движения по направлениям

      // TODO: измерять средний за 10 сек. fps, и пропорционально менять скорость перемещения
      float speed_rotate = 0.001f; // скорость поворота (радиан в секунду) камеры
      float speed_moving = 120.f;   // скорость перемещения (в секунду) камеры

      float vision_angle = 50.f;   // угол зрения для расчета матрицы проекции
      float fovy = (hPi/90.f)*vision_angle;
      float zNear = 1.f;          // расстояние до ближней плоскости матрицы проекции
      float zFar  = 10000.f;      // расстояние до дальней плоскости матрицы проекции
      glm::mat4 MatProjection {}; // матрица проекции 3D сцены
      glm::mat4 MatMVP  {};       // Матрица преобразования
      glm::mat4 MatView {};       // матрица вида
      glm::vec3
        UpWard {0.0, -1.0, 0.0},  // направление наверх
        ViewTo {};                // направление взгляда

      void load_surf_textures(void);
      void calc_position(void);
      void program_3d_init(void);
  };

} //namespace
#endif
