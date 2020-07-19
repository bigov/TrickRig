/**
 *
 * file: space.cpp
 *
 * Управление виртуальным 3D пространством
 *
 * Коды нажатия клавиш:
 *   Ctrl: 285, S: 31
 *
 */

#include "space.hpp"
#include "config.hpp"

namespace tr
{

///
/// \brief space::space
/// \details Формирование 3D пространства
///
space::space(std::shared_ptr<trgl>& pGl): OGLContext(pGl)
{
  render_indices.store(0);
  ViewFrom = std::make_shared<glm::vec3> ();

  light_direction = glm::normalize(glm::vec3(0.3f, 0.45f, 0.4f)); // направление (x,y,z)
  light_bright = glm::vec3(0.99f, 0.99f, 1.00f);                  // цвет        (r,g,b)

  glClearColor(0.5f, 0.69f, 1.0f, 1.0f);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  glFrontFace(GL_CCW);
  glCullFace(GL_BACK);

  //glEnable(GL_CULL_FACE); // отключить отображение обратных поверхностей
  glDisable(GL_CULL_FACE);  // обратные поверхности отображать

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LINE_SMOOTH);
  glEnable(GL_BLEND);      // поддержка прозрачности
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  init_prog_3d();
  init_prog_2d();


  // настройка рендер-буфера
  GLsizei width, height;
  OGLContext->get_frame_size(&width, &height);

  ImHUD.resize( static_cast<uint>(width), static_cast<uint>(height));

  RenderBuffer = std::make_unique<frame_buffer> ();
  if(!RenderBuffer->init(width, height)) ERR("Error on creating Render Buffer.");

  // загрузка текстур
  load_textures();

  OGLContext->add_size_observer(*this); //пересчет при изменении размера

  init_buffers();
}


///
/// \brief space::init_prog_3d
///
void space::init_prog_3d(void)
{
  std::list<std::pair<GLenum, std::string>> Shaders {};
  Shaders.push_back({ GL_VERTEX_SHADER, cfg::app_key(SHADER_VERT_SCENE) });
  Shaders.push_back({ GL_FRAGMENT_SHADER, cfg::app_key(SHADER_FRAG_SCENE) });

  Program3d = std::make_unique<glsl>(Shaders);
  Program3d->use();

  // Заполнить список атрибутов GLSL программы
  Program3d->AtribsList.push_back(
    { Program3d->attrib("position"), 3, GL_FLOAT, GL_FALSE,bytes_per_vertex, 0 * sizeof(GLfloat) });
  Program3d->AtribsList.push_back(
    { Program3d->attrib("color"),    4, GL_FLOAT, GL_TRUE, bytes_per_vertex, 3 * sizeof(GLfloat) });
  Program3d->AtribsList.push_back(
    { Program3d->attrib("normal"),   3, GL_FLOAT, GL_TRUE, bytes_per_vertex, 7 * sizeof(GLfloat) });
  Program3d->AtribsList.push_back(
    { Program3d->attrib("fragment"), 2, GL_FLOAT, GL_TRUE, bytes_per_vertex, 10 * sizeof(GLfloat)});

  glUniform1i(Program3d->uniform("texture_0"), 0);  // glActiveTexture(GL_TEXTURE0)

  Program3d->unuse();
}


///
/// \brief space::init_prog_hud
///
void space::init_prog_2d(void)
{
  // vec2 vPos;
  // vec4 vColor;
  // vec2 vTex;

  static const GLsizeiptr digits_per_dot = 8;
  static const GLsizeiptr bytes_per_dot = digits_per_dot * sizeof(GLfloat);

  std::list<std::pair<GLenum, std::string>> Shaders {};
  Shaders.push_back({ GL_VERTEX_SHADER, "assets\\shaders\\2d_vert.glsl" });
  Shaders.push_back({ GL_FRAGMENT_SHADER, "assets\\shaders\\2d_frag.glsl" });

  Program2d = std::make_unique<glsl>(Shaders);
  Program2d->use();

  Program2d->AtribsList.push_back({Program2d->attrib("vPos"), 2, GL_FLOAT, GL_TRUE, bytes_per_dot, 0 * sizeof(GLfloat)});
  Program2d->AtribsList.push_back({Program2d->attrib("vColor"), 4, GL_FLOAT, GL_TRUE, bytes_per_dot, 2 * sizeof(GLfloat)});
  Program2d->AtribsList.push_back({Program2d->attrib("vTex"), 2, GL_FLOAT, GL_TRUE, bytes_per_dot, 6 * sizeof(GLfloat)});

  glUniform1i(Program2d->uniform("font"), 4);  // glActiveTexture(GL_TEXTURE4)

  Program2d->unuse();

}


///
/// \brief space::init_buffers
/// \details Инициализация VAO и VBO
///
void space::init_buffers(void)
{
  glGenVertexArrays(1, &vao_3d);
  glBindVertexArray(vao_3d);

  // Число сторон куба в объеме с длиной стороны LOD (2*dist_xx) элементов:
  uint n = static_cast<uint>(pow((border_dist_b4 + border_dist_b4 + 1), 3));
  VBOdata3d.allocate(n * bytes_per_face);          // Размер данных VBO для размещения сторон вокселей:
  VBOdata3d.set_attributes(Program3d->AtribsList); // настройка положения атрибутов GLSL программы

  // Так как все четырехугольники сторон индексируются одинаково, то индексный массив
  // заполняем один раз "под завязку" и забываем про него. Число используемых индексов
  // будет всегда соответствовать числу элементов, передаваемых в процедру "glDraw..."
  size_t idx_size = static_cast<size_t>(6 * n * sizeof(GLuint)); // Размер индексного массива
  auto idx_data = std::unique_ptr<GLuint[]> {new GLuint[idx_size]};

  GLuint idx[6] = {0, 1, 2, 2, 3, 0};                                // шаблон четырехугольника
  GLuint stride = 0;                                                 // число описаных вершин
  for(size_t i = 0; i < idx_size; i += 6) {                          // заполнить массив для VBO
    for(size_t x = 0; x < 6; x++) idx_data[x + i] = idx[x] + stride;
    stride += 4;                                                     // по 4 вершины на сторону
  }

  vbo VBOindex { GL_ELEMENT_ARRAY_BUFFER };                          // Индексный буфер
  VBOindex.allocate(static_cast<GLsizei>(idx_size), idx_data.get()); // заполнить данными.
  glBindVertexArray(0);

  glGenVertexArrays(1, &vao_2d);
  glBindVertexArray(vao_2d);

  float data_2d[] = {
    0.0f, 0.0f, .0f, .0f, .0f, 1.0f, 0.0f, 0.0f,
    0.2f, 0.0f, .0f, .0f, .0f, 1.0f, 1.0f, 0.0f,
    0.2f, 0.2f, .0f, .0f, .0f, 1.0f, 1.0f, 1.0f,
    0.2f, 0.2f, .0f, .0f, .0f, 1.0f, 1.0f, 1.0f,
    0.0f, 0.2f, .0f, .0f, .0f, 1.0f, 0.0f, 1.0f,
    0.0f, 0.0f, .0f, .0f, .0f, 1.0f, 0.0f, 0.0f,
  };

  VBOdata2d.allocate( sizeof(data_2d), data_2d );
  VBOdata2d.set_attributes(Program2d->AtribsList);
  glBindVertexArray(0);

  hud_init();
}


///
/// \brief space::enable
///
void space::enable(void)
{
  // Настройка матрицы проекции
  GLsizei width, height;
  OGLContext->get_frame_size(&width, &height);
  auto aspect = static_cast<float>(width) / static_cast<float>(height);
  MatProjection = glm::perspective(fovy, aspect, zNear, zFar);

  xpos = width/2;                          // координаты центра экрана
  ypos = height/2;

  OGLContext->cursor_hide();               // выключить отображение курсора мыши в окне
  OGLContext->set_cursor_pos(xpos, ypos);
  OGLContext->set_cursor_observer(*this);  // Подключить обработчики: курсора мыши
  OGLContext->set_button_observer(*this);  //  -- кнопки мыши

  on_front = 0; // клавиша вперед
  on_back  = 0; // клавиша назад
  on_right = 0; // клавиша вправо
  on_left  = 0; // клавиша влево
  on_up    = 0; // клавиша вверх
  on_down  = 0; // клавиша вниз
  fb_way   = 0; // движение вперед
  ud_way   = 0; // движение вверх
  rl_way   = 0; // движение в сторону

  ready = true;
}


///
/// \brief space::load_map_data
///
void space::map_load(void)
{
  render_indices.store(0);
  // Поток обмена данными с базой
  data_loader = std::make_unique<std::thread>(db_control, OGLContext, ViewFrom, VBOdata3d.get_id(), VBOdata3d.get_size());
}


///
/// \brief space::load_texture
/// \param index
/// \param fname
///
void space::load_textures(void)
{
  // Загрузка текстур поверхностей воксов
  glActiveTexture(GL_TEXTURE0);
  glGenTextures(1, &texture_3d);
  glBindTexture(GL_TEXTURE_2D, texture_3d);
  image ImgTex0 { cfg::app_key(PNG_TEXTURE0) };
  GLint level_of_details = 0;
  GLint frame = 0;
  glTexImage2D(GL_TEXTURE_2D, level_of_details, GL_RGBA,
               static_cast<GLsizei>(ImgTex0.get_width()),
               static_cast<GLsizei>(ImgTex0.get_height()),
               frame, GL_RGBA, GL_UNSIGNED_BYTE, ImgTex0.uchar_t());

  // Установка опций отрисовки
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  glGenerateMipmap(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  // Настройка текстуры для отрисовки HUD
  glActiveTexture(GL_TEXTURE2);
  glGenTextures(1, &texture_hud);
  glBindTexture(GL_TEXTURE_2D, texture_hud);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glBindTexture(GL_TEXTURE_2D, 0);


  // Текстура шрифтов
  glActiveTexture(GL_TEXTURE4);
  glGenTextures(1, &texture_font);
  glBindTexture(GL_TEXTURE_2D, texture_font);

  level_of_details = 0;
  frame = 0;

  glTexImage2D(GL_TEXTURE_2D, level_of_details, GL_RED,
               static_cast<GLsizei>(TextureFont.get_width()),
               static_cast<GLsizei>(TextureFont.get_height()),
               frame, GL_RGBA, GL_UNSIGNED_BYTE, TextureFont.uchar_t());

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
  glGenerateMipmap(GL_TEXTURE_2D);
}


///
/// \brief space::calc_position
/// \param ev
/// \param t: время прорисовки кадра в микросекундах
/// \details Расчет положения и направления движения камеры
///
void space::calc_position(void)
{
  look_dir[0] -= speed_rotate * cursor_dx;
  cursor_dx = 0.f;
  if(look_dir[0] > dPi) look_dir[0] -= dPi;
  if(look_dir[0] < 0) look_dir[0] += dPi;

  look_dir[1] -= speed_rotate * cursor_dy;
  cursor_dy = 0.f;
  if(look_dir[1] > up_max) look_dir[1] = up_max;
  if(look_dir[1] < down_max) look_dir[1] = down_max;

  //if (!space_is_empty(Eye.ViewFrom)) _k *= 0.1f;       // TODO: скорость/туман в воде

  float dist  = speed_moving * static_cast<float>(frame_time); // Дистанция перемещения
  rl = dist * rl_way;
  fb = dist * fb_way;   // по трем нормалям от камеры
  ud = dist * ud_way;

  // промежуточные скаляры для ускорения расчета координат точек вида
  float
    _ca = static_cast<float>(cosf(look_dir[0])),
    _sa = static_cast<float>(sinf(look_dir[0])),
    _ct = static_cast<float>(cosf(look_dir[1]));

  view_mtx.lock();    // смещение камеры за время прошедшее между кадрами
  *ViewFrom.get() += glm::vec3(fb *_ca + rl*sinf(look_dir[0] - Pi), ud,  fb*_sa + rl*_ca);
  view_mtx.unlock();

  ViewTo = *ViewFrom.get() + glm::vec3(_ca*_ct, sinf(look_dir[1]), _sa*_ct); //Направление взгляда

  // Расчет матрицы вида
  MatView = glm::lookAt(*ViewFrom.get(), ViewTo, UpWard);

  // Матрица преобразования
  MatMVP =  MatProjection * MatView;
}


///
/// \brief space::calc_render_time
///
void space::calc_render_time(void)
{
  std::chrono::time_point<sys_clock> t_frame = sys_clock::now();

  static int _fps = 0;
  static std::chrono::time_point<sys_clock> cycle_start = t_frame;
  static std::chrono::time_point<sys_clock> fps_start = t_frame;
  static const std::chrono::seconds one_second(1);

  _fps++;
  if (t_frame - fps_start >= one_second)
  {
    fps_start = t_frame;
    FPS = _fps;
    _fps = 0;
  }

  // время (в секундах) прошедшее после предыдущего вызова данный функции
  frame_time = std::chrono::duration_cast<std::chrono::microseconds>(t_frame - cycle_start).count() / 1000000.0;
  cycle_start = t_frame;
}


///
/// Рендер 3D пространства сцены в буфер "RenderBuffer"
///
/// \details
/// Так как вершины располагаются группами по 4 шт. (обход: 6 индексов / 2 треугольника ),
/// то можно, по номеру (&vertex_id) любой вершины из группы, используя остаток от деления
/// на число вершин в группе, определить какая по номеру из вершин начинает
/// эту группу и какая заканчивает.( hl_point_id_from, hl_point_id_end )
///
void space::render(void)
{
  if(!ready) return;
  if(render_indices.load() < indices_per_face) return;

  calc_render_time();
  calc_position();

  uint vertex_id = 0;    // переменная для приема ID вершины из VBO
  vbo_mtx.lock();

  RenderBuffer->bind();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);

  glBindVertexArray(vao_3d);

  Program3d->use();
  for(const auto& A: Program3d->AtribsList) glEnableVertexAttribArray(A.index);

  Program3d->set_uniform("mvp", MatMVP);                      // матрица пространства
  Program3d->set_uniform("light_direction", light_direction); // направление
  Program3d->set_uniform("light_bright", light_bright);       // цвет/яркость
  Program3d->set_uniform("MinId", hl_vertex_id_from);         // начальная вершина подсветки поверхности
  Program3d->set_uniform("MaxId", hl_vertex_id_end);          // последняя вершина подсветки
  glDrawElements(GL_TRIANGLES, render_indices.load(), GL_UNSIGNED_INT, nullptr);

  for(const auto& A: Program3d->AtribsList) glDisableVertexAttribArray(A.index);
  Program3d->unuse();

  ///
  glBindVertexArray(vao_2d);
  Program2d->use();
  for(const auto& A: Program2d->AtribsList) glEnableVertexAttribArray(A.index);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  for(const auto& A: Program2d->AtribsList) glDisableVertexAttribArray(A.index);
  Program2d->unuse();
  ///

  RenderBuffer->unbind();
  glBindVertexArray(0);

  RenderBuffer->read_pixel(GLint(xpos), GLint(ypos), &vertex_id);
  vbo_mtx.unlock();

  hl_vertex_id_from = vertex_id - (vertex_id % vertices_per_face);
  hl_vertex_id_end = hl_vertex_id_from + vertices_per_face - 1;

  hud_update();
}


///
/// \brief space::resize_event
/// \param width
/// \param height
///
void space::resize_event(int width, int height)
{
  // пересчет матрицы проекции
  auto aspect = static_cast<float>(width) / static_cast<float>(height);
  MatProjection = glm::perspective(fovy, aspect, zNear, zFar);

  // Пересчет размера HUD
  ImHUD.resize( static_cast<uint>(width), static_cast<uint>(height));

  RenderBuffer->resize(width, height);
}


///
/// \brief space::cursor_event
/// \param x
/// \param y
///
void space::cursor_event(double x, double y)
{
  // Накапливаем дистанцию смещения мыши между кадрами рендера.
  // Чем больше значение, тем выше скрость поворота камеры.
  cursor_dx += static_cast<float>(x - xpos);
  cursor_dy += static_cast<float>(y - ypos);

  // После получения значения счетчика восстановить позицию курсора
  OGLContext->set_cursor_pos(xpos, ypos);
}


///
/// \brief space::mouse_event
/// \param _button
/// \param _action
/// \param _mods
///
void space::mouse_event(int _button, int _action, int _mods)
{
  mods   = _mods;
  mouse  = _button;
  action = _action;

  if((hl_vertex_id_end/vertices_per_face)>(render_indices/indices_per_face)) return;

  if((mouse == MOUSE_BUTTON_LEFT) && (action == PRESS))
  {
    action = -1;
    click_side_vertex_id.store(hl_vertex_id_end); // добавление вокса
  }

  if((mouse == MOUSE_BUTTON_RIGHT) && (action == PRESS))
  {
    action = -1;
    click_side_vertex_id.store(-hl_vertex_id_end); // удаление вокса
  }
}


///
/// \brief space::keyboard_event
/// \param _key
/// \param _scancode
/// \param _action
/// \param _mods
///
void space::keyboard_event(int _key, int _scancode, int _action, int _mods)
{
  mouse    = -1;
  key      = _key;
  scancode = _scancode;
  action   = _action;
  mods     = _mods;

  if (PRESS == _action) {
    switch(_key) {
      case KEY_MOVE_UP:
        on_up = 1;
        break;
      case KEY_MOVE_DOWN:
        on_down = 1;
        break;
      case KEY_MOVE_FRONT:
        on_front = 1;
        break;
      case KEY_MOVE_BACK:
        on_back = 1;
        break;
      case KEY_MOVE_LEFT:
        on_left = 1;
        break;
      case KEY_MOVE_RIGHT:
        on_right = 1;
        break;
      default: break;
    }
  } else if (RELEASE == _action) {
    switch(_key) {
      case KEY_MOVE_UP:
        on_up = 0;
        break;
      case KEY_MOVE_DOWN:
        on_down = 0;
        break;
      case KEY_MOVE_FRONT:
        on_front = 0;
        break;
      case KEY_MOVE_BACK:
        on_back = 0;
        break;
      case KEY_MOVE_LEFT:
        on_left = 0;
        break;
      case KEY_MOVE_RIGHT:
        on_right = 0;
        break;
      default: break;
    }
  }

  fb_way = on_front - on_back;
  ud_way = on_down  - on_up;
  rl_way = on_left  - on_right;
}


///
/// \brief загрузка HUD в GPU
///
/// \details Пока HUD имеет упрощенный вид в форме полупрозрачной прямоугольной
/// области в нижней части окна. Эта область формируется в ранее очищеной HUD
/// текстуре окна и зазгружается в память GPU. Загрузка производится разово
/// в момент открытия штроки (cover_) за счет обработки флага "renew". Далее,
/// в процессе взаимодействия с окруженим трехмерной сцены, текструра хранится
/// в памяти. Небольшие локальные фрагменты (вроде FPS-счетчика) обновляются
/// напрямую через память GPU.
///
void space::hud_init(void)
{
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, texture_hud);

  auto width = static_cast<GLint>(ImHUD.get_width());
  auto height = static_cast<GLint>(ImHUD.get_height());

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height,
           0, GL_RGBA, GL_UNSIGNED_BYTE, ImHUD.uchar_t());

  // Панель инструментов для HUD в нижней части окна
  uint h = 48;                           // высота панели инструментов HUD
  if(h > ImHUD.get_height()) h = ImHUD.get_height();   // не может быть выше GuiImg
  image HudPanel {ImHUD.get_width(), h, bg_hud};

  auto y = static_cast<GLint>(ImHUD.get_height() - HudPanel.get_height()); // верхняя граница панели
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, y,
                static_cast<GLsizei>(HudPanel.get_width()), // width
                static_cast<GLsizei>(HudPanel.get_height()), // height
                GL_RGBA, GL_UNSIGNED_BYTE,             // mode
                HudPanel.uchar_t());                   // data
}


///
/// \brief space::hud_draw
///
void space::hud_update(void)
{
  // счетчик FPS
  uchar_color bg = { 0xF0, 0xF0, 0xF0, 0xA0 }; // фон заполнения
  uint fps_length = 4;               // количество символов в надписи
  image Fps {fps_length * TextureFont.get_cell_width() + 4, TextureFont.get_cell_height() + 2, bg};
  char line[5];                       // длина строки с '\0'
  std::sprintf(line, "%.4i", FPS);
  textstring_place(TextureFont, line, Fps, 2, 1);

  vbo_mtx.lock();
  glBindTexture(GL_TEXTURE_2D, texture_hud);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 2, static_cast<GLint>(ImHUD.get_height() - Fps.get_height() - 2),
                static_cast<GLsizei>(Fps.get_width()),  // width
                static_cast<GLsizei>(Fps.get_height()), // height
                GL_RGBA, GL_UNSIGNED_BYTE,              // mode
                Fps.uchar_t());                         // data
  vbo_mtx.unlock();

  // Координаты в пространстве
  uint c_length = 60;               // количество символов в надписи
  image Coord {c_length * TextureFont.get_cell_width() + 4, TextureFont.get_cell_height() + 2, bg};
  char ln[60];                       // длина строки с '\0'
  std::sprintf(ln, "X:%+06.1f, Y:%+06.1f, Z:%+06.1f, a:%+04.3f, t:%+04.3f",
                  ViewFrom->x, ViewFrom->y, ViewFrom->z, look_dir[0], look_dir[1]);
  textstring_place(TextureFont, ln, Coord, 2, 1);

  vbo_mtx.lock();
  glTexSubImage2D(GL_TEXTURE_2D, 0, 2, 2,            // top, left
                static_cast<GLsizei>(Coord.get_width()),  // width
                static_cast<GLsizei>(Coord.get_height()),  // height
                GL_RGBA, GL_UNSIGNED_BYTE,           // mode
                Coord.uchar_t());                      // data
  vbo_mtx.unlock();
}


///
/// \brief space::~space
///
space::~space()
{
  render_indices.store(0); // Индикатор для остановки потока загрузки в рендер из БД
  if(nullptr != data_loader) if(data_loader->joinable())
    data_loader->join();      // Ожидание завершения потока
}

} // namespace tr
