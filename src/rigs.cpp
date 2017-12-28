//============================================================================
//
// file: rigs.cpp
//
// Элементы формирования пространства
//
//============================================================================
#include "rigs.hpp"

namespace tr
{
  //## конструктор
  rig::rig(const tr::snip & Snip): born(get_msec())
  {
    Area.push_front(Snip);
    return;
  }

  //## конструктор
  rig::rig(const tr::f3d &p): born(get_msec())
  {
    add_snip(p);
    return;
  }

  //## конструктор
  rig::rig(int x, int y, int z): born(get_msec())
  {
    add_snip(tr::f3d{x,y,z});
    return;
  }

  //## конструктор копирующий
  rig::rig(const tr::rig & Other): born(get_msec())
  {
  /* При создании нового элемента метку времени
   * ставим по моменту создания объекта
   */
    copy_snips(Other);
    return;
  }

  //## Оператор присваивания это не конструктор
  tr::rig& rig::operator= (const tr::rig & Other)
  {
  /* При копировании существующего элемента в
   * созданый ранее объект метку времени копируем
   */
    if(this != &Other)
    {
      born = Other.born;
      Area.clear();
      copy_snips(Other);
    }
    return *this;
  }

  //## Копирование списка снипов из другого объекта
  void rig::copy_snips(const tr::rig & Other)
  {
    for(tr::snip Snip: Other.Area) Area.push_front(Snip);
    return;
  }

  //## Установка в указаной точке дефолтного снипа
  void rig::add_snip(const tr::f3d &p)
  {
    Area.emplace_front(p);
    return;
  }

  //## КОНСТРУКТОР
  rigs::rigs()
  {
    Prog3d.attach_shaders(
      tr::cfg::get(SHADER_VERT_SCENE),
      tr::cfg::get(SHADER_FRAG_SCENE)
    );
    Prog3d.use();

    // инициализация VAO
    glGenVertexArrays(1, &space_vao);
    glBindVertexArray(space_vao);

    // Число элементов в кубе с длиной стороны = "space_i0_length" элементов:
    unsigned n = pow((tr::lod_0 + tr::lod_0 + 1), 3);

    // число байт для заполнения такого объема прямоугольниками:
    VBOdata.allocate(n * tr::bytes_per_snip);
    // настройка положения атрибутов
    VBOdata.attrib(Prog3d.attrib_location_get("position"),
      4, GL_FLOAT, GL_FALSE, tr::bytes_per_vertex, (void*)(0 * sizeof(GLfloat)));
    VBOdata.attrib(Prog3d.attrib_location_get("color"),
      4, GL_FLOAT, GL_TRUE, tr::bytes_per_vertex, (void*)(4 * sizeof(GLfloat)));
    VBOdata.attrib(Prog3d.attrib_location_get("normal"),
      4, GL_FLOAT, GL_TRUE, tr::bytes_per_vertex, (void*)(8 * sizeof(GLfloat)));
    VBOdata.attrib(Prog3d.attrib_location_get("fragment"),
      2, GL_FLOAT, GL_TRUE, tr::bytes_per_vertex, (void*)(12 * sizeof(GLfloat)));

    //
    // Так как все четырехугольники в снипах индексируются одинаково, то индексный массив
    // заполняем один раз "под завязку" и забываем про него. Число используемых индексов
    // будет всегда соответствовать числу элементов, передаваемых в процедру "glDraw..."
    //
    size_t idx_size = static_cast<size_t>(6 * n * sizeof(GLuint)); // Размер индексного массива
    GLuint *idx_data = new GLuint[idx_size];                       // данные для заполнения
    GLuint idx[6] = {0, 1, 2, 2, 3, 0};                            // шаблон четырехугольника
    GLuint stride = 0;                                             // число описаных вершин
    for(size_t i=0; i < idx_size; i += 6) {                        // заполнить массив для VBO
      for(size_t x = 0; x < 6; x++) idx_data[x + i] = idx[x] + stride;
      stride += 4;                                                 // по 4 вершины на снип
    }
    tr::vbo VBOindex = {GL_ELEMENT_ARRAY_BUFFER};                  // Создать индексный буфер
    VBOindex.allocate(idx_size, idx_data);                         // и заполнить данными.
    delete[] idx_data;                                             // Удалить исходный массив.

    glBindVertexArray(0);
    Prog3d.unuse();

    return;

  }

  //##  Рендер кадра
  void rigs::draw(const glm::mat4 & MVP)
  {
    if (!CachedOffset.empty()) clear_cashed_snips();

    Prog3d.use();   // включить шейдерную программу
    Prog3d.set_uniform("mvp", MVP);
    Prog3d.set_uniform("light_direction", glm::vec4(0.2f, 0.9f, 0.5f, 0.0));
    Prog3d.set_uniform("light_bright", glm::vec4(0.5f, 0.5f, 0.5f, 0.0));

    glBindVertexArray(space_vao);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDrawElements(GL_TRIANGLES, render_points, GL_UNSIGNED_INT, NULL);
    glBindVertexArray(0);
    Prog3d.unuse(); // отключить шейдерную программу

    return;
  }

  //## Размещение элементов в графическом буфере
  void rigs::show(const tr::f3d & P)
  {
    /* Индексы размещенных в VBO данных, которые при перемещении камеры вышли
     * за границу отображения, запоминаются в кэше, чтобы на их место
     * записать данные вершин, которые вошли в поле зрения с другой стороны.
     */

      tr::rig * Rig = get(P);
      if(nullptr == Rig) return;

      for(tr::snip & Snip: Rig->Area)
      {
        bool data_is_recieved = false;
        while (!data_is_recieved)
        {
          if(CachedOffset.empty()) // Если кэш пустой, то добавляем данные в конец VBO
          {
            Snip.vbo_append(VBOdata);
            render_points += tr::indices_per_snip;  // увеличить число точек рендера
            VisibleSnips[Snip.data_offset] = &Snip; // добавить ссылку
            data_is_recieved = true;
          }
          else // если в кэше есть адреса свободных мест, то используем
          {    // их с контролем, успешно ли был перемещен блок данных
            data_is_recieved = Snip.vbo_update(VBOdata, CachedOffset.front());
            CachedOffset.pop_front();                // укоротить кэш
            if(data_is_recieved) VisibleSnips[Snip.data_offset] = &Snip; // добавить ссылку
          }
        }
      }
      Rig->in_vbo = true;
      return;
  }

  //## убрать риг из рендера
  void rigs::hide(const tr::f3d & P)
  {
    tr::rig * Rig = get(P);
    if(nullptr == Rig) return;

    #ifndef NDEBUG
    if(!Rig->in_vbo)
    {
      tr::info("rigs::hide try to hide hidden Rig.");
      return;
    }
    #endif

    for(auto & Snip: Rig->Area)
    {
      VisibleSnips.erase(Snip.data_offset);
      CachedOffset.push_front(Snip.data_offset);
    }
    Rig->in_vbo = false;
    return;
  }

  //## Удаление элементов по адресам с кэше и сжатие данных в VBO
  void rigs::clear_cashed_snips(void)
  {
  /* Если в кэше есть адрес в середине VBO, то на него переносим данные
   * из конца и сжимаем буфер на один блок. Если адрес в кэше расположен
   * в конце VBO, то сжимаем буфер сдвигая границу и уменьшая число
   * элементов в рендере.
   */

    // Выбрать самый крайний элемент VBO на границе блока данных
    GLsizeiptr data_src = VBOdata.get_hem();

    #ifndef NDEBUG
    if(data_src == 0 ) {      // Если (вдруг!) данных нет, то
      CachedOffset.clear();    // очистить все, сообщить о проблеме
      VisibleSnips.clear();   // и закончить обработку кэша
      render_points = 0;
      info("WARNING: space::clear_cashed_snips got empty data_src\n");
      return; }

    /// Граница буфера (VBOdata.get_hem()), если она не равна нулю,
    /// не может быть меньше размера блока данных (bytes_per_snip)
    if(tr::bytes_per_snip > data_src)
        ERR ("BUG!!! space::jam_vbo got error address in VBO");
    #endif

    data_src -= tr::bytes_per_snip; // адрес последнего блока

    // Если крайний блок не в списке VisibleSnips, то он и не в рендере.
    // Поэтому просто отбросим его, сдвинув границу буфера VBO. Кэш не
    // изменяем, так как в контейнере "forward_list" удаление элементов
    // из середины списка - затратная операция.
    //
    // Внимание! Так как после этого где-то в кэше остается невалидный
    // (за рабочей границей VBO) адрес блока, то при использовании
    // адресов из кэша надо делать проверку - не "протухли" ли они.
    //
    if(VisibleSnips.find(data_src) == VisibleSnips.end())
    {
      VBOdata.shrink(tr::bytes_per_snip);   // укоротить VBO данных
      render_points -= tr::indices_per_snip; // уменьшить число точек рендера
      return;                                // и прервать обработку кэша
    }

    // Извлечь из кэша один освободившийся адрес
    GLsizeiptr data_dst = CachedOffset.front(); CachedOffset.pop_front();
    // если извлеченный адрес за границей VBO, то прервать обработку
    if(data_dst >= data_src) return;

    try { // Если есть отображаемый data_src и меньший data_dst из кэша, то
      tr::snip *Snip = VisibleSnips.at(data_src);  // сжать буфер VBO,
      Snip->vbo_jam(VBOdata, data_dst); // переместив снип,
      VisibleSnips[Snip->data_offset] = Snip;      // обновить ссылку и
      render_points -= tr::indices_per_snip;       // уменьшить число точек рендера
    } catch (...) {
      ERR("space::jam_vbo got error VisibleSnips[data_src]");
    }

    return;
  }

  //## Установка масштаба и загрузка пространства из БД
  void rigs::init(float g)
  {
  // Метод обеспечивает формирование в оперативной памяти приложения
  // карту ( контейнер map() ) размещения ригов в трехмерных координатах для
  // назначеной области пространства. Данные размещаемые в VBO берутся
  // из этого контейнера.

    db_gage = g; // TODO проверка масштаба на допустимость


    /// Загрузка из Obj файла объекта в один риг:
    //tr::obj_load Obj = {"../assets/test_flat.obj"};
    //tr::f3d P = {1.f, 0.f, 1.f};
    //for(auto &S: Obj.Area) S.point_set(P); // Установить объект в точке P
    //get(P)->Area.swap(Obj.Area);           // Загрузить в rig(P)

    // загрузка из Obj файла по одной плоскости в каждый риг
    tr::obj_load Obj = {"../assets/surf16x16.obj"};

    tr::f3d Base = {0.0f, 0.0f, 0.0f};
    tr::f3d Pt   = {0.0f, 0.0f, 0.0f};

    for(int z = -4; z < 4; z ++)
    {
      Base.z = static_cast<float>(z * 16 + 8);

    for(int x = -4; x < 4; x ++)
    {
      Base.x = static_cast<float>(x * 16 + 8);

      for(tr::snip S: Obj.Area)
      {
        size_t n = 0;
        // В снипе 4 вершины. Найдем индекс опорной (по минимальному значению)
        for(size_t i = 1; i < 4; i++)
          if((
               S.data[n * ROW_STRIDE + COORD_X]
             + S.data[n * ROW_STRIDE + COORD_Y]
             + S.data[n * ROW_STRIDE + COORD_Z]
                ) > (
               S.data[i * ROW_STRIDE + COORD_X]
             + S.data[i * ROW_STRIDE + COORD_Y]
             + S.data[i * ROW_STRIDE + COORD_Z]
             )) n = i;

        // Координаты найденой вершины используем для создания рига
        Pt.x = floor(S.data[n * ROW_STRIDE + COORD_X]) + Base.x;
        Pt.y = floor(S.data[n * ROW_STRIDE + COORD_Y]) + Base.y;
        Pt.z = floor(S.data[n * ROW_STRIDE + COORD_Z]) + Base.z;

        S.point_set(Pt);
        Db[Pt] = tr::rig { S };
      }
    } //for x
    } //for z

    // Выделить текстурой центр координат
    get(0,0,0 )->Area.front().texture_set(0.125, 0.125 * 4.0);

    return;
  }

  //## Проверка наличия блока в заданных координатах
  bool rigs::exist(float x, float y, float z)
  {
    if (nullptr == get(x, y, z)) return false;
    else return true;
  }

  //## поиск ближайшего нижнего блока по координатам точки
  tr::f3d rigs::search_down(const glm::vec3& v)
  {
  // При работе изменяет значение полученного аргумента.
  // Если объект найден, то аргумен содержит его координаты
  //
    return search_down(v.x, v.y, v.z);
  }

  //## поиск ближайшего нижнего блока по координатам точки
  tr::f3d rigs::search_down(float x, float y, float z)
  {
    if(y < yMin) ERR("Y downflow"); if(y > yMax) ERR("Y overflow");

    x = floor(x); y = floor(y); z = floor(z);

    while(y > yMin)
    {
      try
      { 
        Db.at(f3d {x, y, z});
        return f3d {x, y, z};
      } catch (...)
      { y -= db_gage; }
    }
    ERR("Rigs::search_down() failure. You need try/catch in this case.");
  }

  //## Поиск элемента с указанными координатами
  rig* rigs::get(float x, float y, float z)
  {
    return get(tr::f3d{x, y, z});
  }

  //## Поиск элемента с указанными координатами
  rig* rigs::get(const tr::f3d &P)
  {
    if(P.y < yMin) ERR("rigs::get -Y is overflow");
    if(P.y > yMax) ERR("rigs::get +Y is overflow");
    // Вначале поищем как указано.
    try { return &Db.at(P); }
    catch (...) { return nullptr; }
  }

} //namespace
