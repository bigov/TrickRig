/**
 *
 * file: area.cpp
 *
 * Класс управления элементами области 3D пространства
 *
 */

#include "area.hpp"

namespace tr
{


///
/// \brief area::area
/// \param length - длина стороны вокселя
/// \param count - число вокселей от камеры (или внутренней границы)
/// до внешней границы области
///
area::area(int side_length, int count_elements_to_border, vbo_ext* v)
{
  vox_side_len = side_length;
  lod_dist_far = count_elements_to_border * side_length;

  // Origin вокселя, в котором расположена камера
  Location = { static_cast<int>(floorf(Eye.ViewFrom.x / vox_side_len)) * vox_side_len,
               static_cast<int>(floorf(Eye.ViewFrom.y / vox_side_len)) * vox_side_len,
               static_cast<int>(floorf(Eye.ViewFrom.z / vox_side_len)) * vox_side_len };
  MoveFrom = Location;

  i3d P0 { Location.x - lod_dist_far,
           Location.y - lod_dist_far,
           Location.z - lod_dist_far };
  i3d P1 { Location.x + lod_dist_far,
           Location.y + lod_dist_far,
           Location.z + lod_dist_far };

  VoxBuffer = std::make_unique<vox_buffer> (vox_side_len, v, P0, P1);
}


///
/// \brief area::append
/// \param i
///
void area::append(u_int i)
{
  VoxBuffer->append(i);
}


///
/// \brief area::remove
/// \param i
///
void area::remove(u_int i)
{
  VoxBuffer->remove(i);
}

///
/// \brief area::render_indices
/// \return
/// \details Возвращает число индексов для функции вызова рендера
///
u_int area::render_indices()
{
  return VoxBuffer->get_render_indices();
}


///
/// \brief space::redraw_borders_x
/// \details Построение границы области по оси X по ходу движения
///
void area::redraw_borders_x(void)
{
  int yMin = -lod_dist_far;              // Y-граница LOD
  int yMax =  lod_dist_far;

  int x_show, x_hide;
  if(Location.x > MoveFrom.x)
  {        // Если направление движение по оси Х
    x_show = Location.x + lod_dist_far; // X-линия вставки вокселей на границе LOD
    x_hide = MoveFrom.x - lod_dist_far; // X-линия удаления вокселей на границе
  } else { // Если направление движение против оси Х
    x_show = Location.x - lod_dist_far; // X-линия вставки вокселей на границе LOD
    x_hide = MoveFrom.x + lod_dist_far; // X-линия удаления вокселей на границе
  }

  int zMin, zMax;

  // Скрыть элементы с задней границы области
  zMin = MoveFrom.z - lod_dist_far;
  zMax = MoveFrom.z + lod_dist_far;
  for(int y = yMin; y <= yMax; y += vox_side_len)
    for(int z = zMin; z <= zMax; z += vox_side_len) QueueWipe.push({x_hide, y, z});

  // Добавить линию элементов по направлению движения
  zMin = Location.z - lod_dist_far;
  zMax = Location.z + lod_dist_far;
  for(int y = yMin; y <= yMax; y += vox_side_len)
    for(int z = zMin; z <= zMax; z += vox_side_len) QueueLoad.push({x_show, y, z});

  MoveFrom.x = Location.x;
}


///
/// \brief space::redraw_borders_z
/// \details Построение границы области по оси Z по ходу движения
///
void area::redraw_borders_z(void)
{
  int yMin = -lod_dist_far;              // Y-граница LOD
  int yMax =  lod_dist_far;

  int z_show, z_hide;
  if(Location.z > MoveFrom.z)
  {        // Если направление движение по оси Z
    z_show = Location.z + lod_dist_far; // Z-линия вставки вокселей на границе LOD
    z_hide = MoveFrom.z - lod_dist_far; // Z-линия удаления вокселей на границе
  } else { // Если направление движение против оси Z
    z_show = Location.z - lod_dist_far; // Z-линия вставки вокселей на границе LOD
    z_hide = MoveFrom.z + lod_dist_far; // Z-линия удаления вокселей на границе
  }

  int xMin, xMax;

  // Скрыть элементы с задней границы области
  xMin = MoveFrom.x - lod_dist_far;
  xMax = MoveFrom.x + lod_dist_far;
  for(int y = yMin; y <= yMax; y += vox_side_len)
    for(int x = xMin; x <= xMax; x += vox_side_len) QueueWipe.push({x, y, z_hide});

  // Добавить линию элементов по направлению движения
  xMin = Location.x - lod_dist_far;
  xMax = Location.x + lod_dist_far;
  for(int y = yMin; y <= yMax; y += vox_side_len)
    for(int x = xMin; x <= xMax; x += vox_side_len) QueueLoad.push({x, y, z_show});

  MoveFrom.z = Location.z;
}


///
/// \brief space::recalc_borders
/// \details Перестроение границ активной области при перемещении камеры
///
/// TODO? (на случай притормаживания - если прыгать камерой туда-сюда через
/// границу запуска перерисовки границ) можно процедуры "redraw_borders_?"
/// разбить по две части - вперед/назад.
///
void area::recalc_borders(void)
{
  // Origin вокселя, в котором расположена камера
  Location = { static_cast<int>(floorf(Eye.ViewFrom.x / vox_side_len)) * vox_side_len,
               static_cast<int>(floorf(Eye.ViewFrom.y / vox_side_len)) * vox_side_len,
               static_cast<int>(floorf(Eye.ViewFrom.z / vox_side_len)) * vox_side_len };

  if(Location.x != MoveFrom.x) redraw_borders_x();
  if(Location.z != MoveFrom.z) redraw_borders_z();

  queue_release();
}


///
/// \brief area::queue_release
///
/// \details Операции с базой данных по скорости гораздо медленнее, чем рендер объектов из
/// оперативной памяти, поэтому обмен информацией с базой данных и выгрузка больших объемов
/// данных из памяти (при перестроении границ LOD) производится покадрово через очередь
/// фиксированными по продолжительности порциями.
void area::queue_release(void)
{
  auto st = std::chrono::system_clock::now();
  while ((std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - st).count() < 5))
  {
    // В работе постоянно две очереди: по ходу движения камеры очередь загрузки,
    // а с противоположной стороны - очередь выгрузки элементов.

    // Загрузка элементов пространства в графический буфер
    if(!QueueLoad.empty())
    {
      VoxBuffer->vox_load(QueueLoad.front());  // Загрузка данных имеет приоритет. Пока вся очередь
      QueueLoad.pop();                        // загрузки не очистится, очередь выгрузки ждет.
      continue;
    }

    // Удаление из памяти данных тех элементов, которые в результате перемещения
    // камеры вышли за границу отображения текущего LOD
    if(!QueueWipe.empty())
    {
      VoxBuffer->vox_unload(QueueWipe.front());
      QueueWipe.pop();
    } else { return; }
  }
}


} //namespace
