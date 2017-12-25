//============================================================================
//
// file: scene.cpp
//
// Управление пространством 3D сцены
//
//============================================================================
#include "space.hpp"

namespace tr
{
  //## Формирование 3D пространства
  space::space(void)
  {
    RigsDb0.init(g0); // загрузка уровня LOD-0
    vbo_allocate_mem();

    glClearColor(0.5f, 0.69f, 1.0f, 1.0f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);
    //glEnable(GL_CULL_FACE);  // после загрузки сцены опция выключается
    glDisable(GL_CULL_FACE); // включить отображение обратных поверхностей
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);      // поддержка прозрачности
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Загрузка из файла данных текстуры
    image image = get_png_img(tr::Cfg.get(PNG_TEXTURE0));

    Eye.look_a = std::stof(tr::Cfg.get(LOOK_AZIM));
    Eye.look_t = std::stof(tr::Cfg.get(LOOK_TANG));

    glGenTextures(1, &m_textureObj);
    glActiveTexture(GL_TEXTURE0); // можно загрузить не меньше 48
    glBindTexture(GL_TEXTURE_2D, m_textureObj);

    GLint level_of_details = 0;
    GLint frame = 0;
    glTexImage2D(GL_TEXTURE_2D, level_of_details, GL_RGBA,
      image.w, image.h, frame, GL_RGBA, GL_UNSIGNED_BYTE, image.Data.data());

    // Установка опций отрисовки текстур
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
      GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    upload_vbo();
    return;
  }

  //## Загрузка в VBO (графическую память) данных отображаемых объектов 3D сцены
  void space::upload_vbo(void)
  {
  // TODO: тут должны загружаться в графическую память все LOD_*,
  //       но пока загружается только LOD_0

    f3d pt = RigsDb0.search_down(tr::Eye.ViewFrom); // ближайший к камере снизу блок

    // используется в функциях пересчета границ отрисовки областей
    MoveFrom = {floor(pt.x), floor(pt.y), floor(pt.z)};

    float // границы уровня lod_0
      xMin = MoveFrom.x - ceil(tr::lod_0),
      yMin = MoveFrom.y - ceil(tr::lod_0),
      zMin = MoveFrom.z - ceil(tr::lod_0),
      xMax = MoveFrom.x + ceil(tr::lod_0),
      yMax = MoveFrom.y + ceil(tr::lod_0),
      zMax = MoveFrom.z + ceil(tr::lod_0);

    // Загрузить в графический буфер атрибуты элементов
    for(float x = xMin; x<= xMax; x += g0)
    for(float y = yMin; y<= yMax; y += g0)
    for(float z = zMin; z<= zMax; z += g0)
      if(RigsDb0.exist(x, y, z)) vbo_data_send(x, y, z);

    return;
  }

  //## запись данных в графический буфер
  void space::vbo_data_send(float x, float y, float z)
  {
  /* Индексы размещенных в VBO данных, которые при перемещении камеры вышли
   * за границу отображения, запоминаются в списке CasheVBOptr, чтобы на их место
   * записать данные точек, которые вошли в поле зрения с другой стороны.
   */

    tr::rig* Rig = RigsDb0.get(x, y, z);
    #ifndef NDEBUG
    if(nullptr == Rig) ERR("ERR: call post_key for empty space.");
    #endif

    glBindVertexArray(space_vao);

    for(tr::snip & Snip: Rig->Area)
    {
      bool data_is_recieved = false;
      while (!data_is_recieved)
      {
        if(CashedSnips.empty()) // Если кэш пустой, то добавляем данные в конец VBO
        {
          Snip.vbo_append(VBOdata, VBOindex);
          render_points += tr::indices_per_snip;  // увеличить число точек рендера
          VisibleSnips[Snip.data_offset] = &Snip; // добавить ссылку
          data_is_recieved = true;
        }
        else // если в кэше есть адреса свободных мест, то используем
        {    // их с контролем, успешно ли был перемещен блок данных
          data_is_recieved = Snip.vbo_update(VBOdata, VBOindex, CashedSnips.front());
          CashedSnips.pop_front();                // укоротить кэш
          VisibleSnips[Snip.data_offset] = &Snip; // добавить ссылку
        }
      }
    }

    glBindVertexArray(0);
    return;
  }

  //## Инициализировать, настроить VBO и заполнить данными
  void space::vbo_allocate_mem(void)
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
    VBOdata.allocate(n * tr::snip_data_bytes);
    // настройка положения атрибутов
    VBOdata.attrib(Prog3d.attrib_location_get("position"),
      4, GL_FLOAT, GL_FALSE, tr::snip_bytes_per_vertex, (void*)(0 * sizeof(GLfloat)));
    VBOdata.attrib(Prog3d.attrib_location_get("color"),
      4, GL_FLOAT, GL_TRUE, tr::snip_bytes_per_vertex, (void*)(4 * sizeof(GLfloat)));
    VBOdata.attrib(Prog3d.attrib_location_get("normal"),
      4, GL_FLOAT, GL_TRUE, tr::snip_bytes_per_vertex, (void*)(8 * sizeof(GLfloat)));
    VBOdata.attrib(Prog3d.attrib_location_get("fragment"),
      2, GL_FLOAT, GL_TRUE, tr::snip_bytes_per_vertex, (void*)(12 * sizeof(GLfloat)));

    // индексный массив
    VBOindex.allocate(static_cast<size_t>(6 * n * sizeof(GLuint)));

    glBindVertexArray(0);
    Prog3d.unuse();

    return;
  }

  //## Построение границы области по оси X по ходу движения
  // TODO: ВНИМАНИЕ! проверяется только 10 слоев по Y
  void space::redraw_borders_x()
  {
    rig* Rig;
    float
      yMin = -5.f, yMax =  5.f, // Y границы области сбора
      x_old, x_new,  // координаты линий удаления/вставки новых фрагментов
      vf_x = floor(tr::Eye.ViewFrom.x), vf_z = floor(tr::Eye.ViewFrom.z),
      clod_0 = ceil(tr::lod_0);

    if(MoveFrom.x > vf_x) {
      x_old = MoveFrom.x + clod_0;
      x_new = vf_x - clod_0;
    } else {
      x_old = MoveFrom.x - clod_0;
      x_new = vf_x + clod_0;
    }

    // Сбор индексов VBO с задней границы области
    float zMin = MoveFrom.z - clod_0, zMax = MoveFrom.z + clod_0;
    for(float y = yMin; y <= yMax; y += g0)
    for(float z = zMin; z <= zMax; z += g0)
    {
      Rig = RigsDb0.get(x_old, y, z);
      if(nullptr != Rig) for(auto & Snip: Rig->Area)
      {
        VisibleSnips.erase(Snip.data_offset);
        CashedSnips.push_front(Snip.data_offset);
      }
    }

    // Построение линии по направлению движения
    zMin = vf_z - clod_0; zMax = vf_z + clod_0;
    for(float y = yMin; y <= yMax; y += g0)
      for(float z = zMin; z <= zMax; z += g0)
        if(RigsDb0.exist(x_new, y, z)) vbo_data_send(x_new, y, z);

    MoveFrom.x = vf_x;
    return;
  }

  //## Построение границы области по оси Z по ходу движения
  // TODO: ВНИМАНИЕ! проверяется только 10 слоев по Y
  void space::redraw_borders_z()
  {
    tr::rig* Rig;
    float
      yMin = -5.f, yMax =  5.f, // Y границы области сбора
      z_old, z_new,  // координаты линий удаления/вставки новых фрагментов
      vf_z = floor(tr::Eye.ViewFrom.z), vf_x = floor(tr::Eye.ViewFrom.x),
      clod_0 = ceil(tr::lod_0);

    if(MoveFrom.z > vf_z) {
      z_old = MoveFrom.z + clod_0;
      z_new = vf_z - clod_0;
    } else {
      z_old = MoveFrom.z - clod_0;
      z_new = vf_z + clod_0;
    }

    // Сбор индексов VBO с задней границы области
    float xMin = MoveFrom.x - clod_0, xMax = MoveFrom.x + clod_0;
    for(float y = yMin; y <= yMax; y += g0)
    for(float x = xMin; x <= xMax; x += g0)
    {
      Rig = RigsDb0.get(x, y, z_old);
      if(nullptr != Rig) for(auto & Snip: Rig->Area)
      {
        VisibleSnips.erase(Snip.data_offset);
        CashedSnips.push_front(Snip.data_offset);
      }
    }

    // Построение линии по направлению движения
    xMin = vf_x - clod_0; xMax = vf_x + clod_0;
    for(float y = yMin; y <= yMax; y += g0)
      for(float x = xMin; x <= xMax; x += g0)
        if(RigsDb0.exist(x, y, z_new)) vbo_data_send(x, y, z_new);

    MoveFrom.z = vf_z;
    return;
  }

  //## Перестроение границ активной области при перемещении камеры
  void space::recalc_borders(void)
  {
  /* - собираем в кэш адреса удаляемых снипов.
   * - добавляемые в сцену снипы пишем в VBO по адресам из кэша.
   * - если кэш пустой, то добавляем в конец буфера.
   *
   * TODO? (на случай притормаживания если прыгать камерой туда-сюда
   * через границу запуска перерисовки границ)
   *   Можно процедуры "redraw_borders_?" разбить по две части - отдельно
   *   сбор данных в кэш и отдельно построение новой границы по ходу движения.
   *
   * - запускать их с небольшим зазором (вначале сбор, потом перестроение)
   *
   * - в процедуре построения вначале проверять наличие снипов рига в кэше
   *   и просто активировать их без запроса к базе данных, если они там.
   *
   * Еще можно кэшировать данные в обработчике соединений с базой данных,
   * сохраняя в нем данные, например, о двух линиях одновременно по внешнему
   * периметру. Это должно снизить число обращений к (диску) базе данных при
   * резких маятниковых перемещениях камеры.
   */

    if(floor(tr::Eye.ViewFrom.x) != MoveFrom.x) redraw_borders_x();
    //if(floor(tr::Eye.ViewFrom.y) != MoveFrom.y) redraw_borders_y();
    if(floor(tr::Eye.ViewFrom.z) != MoveFrom.z) redraw_borders_z();
    return;
  }

  //## Удаление элементов по адресам с кэше и сжатие данных в VBO
  void space::clear_cashed_snips(void)
  {
  /* Если в кэше есть адреса из середины VBO, то на них переносим данные
   * из конца и сжимаем буфер на один блок. Если адрес в кэше из конца VBO,
   * то сразу сжимаем буфер, просто отбрасывая уже ненужный блок.
   */

    // Выбрать самый крайний элемент VBO на границе блока данных
    GLsizeiptr data_src = VBOdata.get_hem();

    #ifndef NDEBUG
    if(data_src == 0 ) {      // если данных нет (а вдруг?)
      CashedSnips.clear();    // то очистить все, сообщить о проблеме
      VisibleSnips.clear();   // и закончить обработку кэша
      render_points = 0;
      info("WARNING: space::clear_cashed_snips got empty data_src\n");
      return; }
    #endif

    data_src -= tr::snip_data_bytes; // адрес последнего блока

    #ifndef NDEBUG
      if(data_src < 0) ERR ("space::jam_vbo got error address in VBO");
    #endif

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
      VBOdata.shrink(tr::snip_data_bytes);   // укоротить VBO данных
      VBOindex.shrink(tr::snip_index_bytes); // укоротить VBO индекса
      render_points -= tr::indices_per_snip; // уменьшить число точек рендера
      return;                                // и прервать обработку кэша
    }

    // Извлечь из кэша один освободившийся адрес
    GLsizeiptr data_dst = CashedSnips.front(); CashedSnips.pop_front();
    // если извлеченный адрес за границей VBO, то прервать обработку
    if(data_dst >= data_src) return;

    try { // Если есть отображаемый data_src и меньший data_dst из кэша, то
      tr::snip *Snip = VisibleSnips.at(data_src);  // сжать буфер VBO,
      Snip->jam_data(VBOdata, VBOindex, data_dst); // переместив снип,
      VisibleSnips[Snip->data_offset] = Snip;      // обновить ссылку и
      render_points -= tr::indices_per_snip;       // уменьшить число точек рендера
    } catch (...) {
      ERR("space::jam_vbo got error VisibleSnips[data_src]");
    }

    return;
  }

  //## Расчет положения и направления движения камеры
  void space::calc_position(const evInput & ev)
  {
    Eye.look_a += ev.dx * Eye.look_speed;
    if(Eye.look_a > two_pi) Eye.look_a -= two_pi;
    if(Eye.look_a < 0) Eye.look_a += two_pi;

    Eye.look_t -= ev.dy * Eye.look_speed;
    if(Eye.look_t > look_up) Eye.look_t = look_up;
    if(Eye.look_t < look_down) Eye.look_t = look_down;

    float _k = Eye.speed / static_cast<float>(ev.fps); // корректировка по FPS

    //if (!space_is_empty(tr::Eye.ViewFrom)) _k *= 0.1f;       // TODO: скорость/туман в воде

    rl = _k * static_cast<float>(ev.rl);   // скорости движения
    fb = _k * static_cast<float>(ev.fb);   // по трем осям
    ud = _k * static_cast<float>(ev.ud);

    // промежуточные скаляры для ускорения расчета координат точек вида
    float
      _ca = static_cast<float>(cos(Eye.look_a)),
      _sa = static_cast<float>(sin(Eye.look_a)),
      _ct = static_cast<float>(cos(Eye.look_t));

    glm::vec3 LookDir {_ca*_ct, sin(Eye.look_t), _sa*_ct}; //Направление взгляда
    tr::Eye.ViewFrom += glm::vec3(fb *_ca + rl*sin(Eye.look_a - pi), ud,  fb*_sa + rl*_ca);
    ViewTo = tr::Eye.ViewFrom + LookDir;

    // Расчет матрицы вида
    MatView = glm::lookAt(tr::Eye.ViewFrom, ViewTo, UpWard);

    calc_selected_area(LookDir);
    return;
  }

  //## Расчет координат ближнего блока, на который направлен взгляд
  void space::calc_selected_area(glm::vec3 & s_dir)
  {
     Selected = tr::Eye.ViewFrom - s_dir;
     return;

   /*               ******** ! отключено ! ********             */

    Selected = ViewTo;
    glm::vec3 check_step = { s_dir.x/8.f, s_dir.y/8.f, s_dir.z/8.f };
    for(int i = 0; i < 24; ++i)
    {
      if(RigsDb0.exist(Selected.x, Selected.y, Selected.z))
      {
        Selected.x = floor(Selected.x);
        Selected.y = floor(Selected.y);
        Selected.z = floor(Selected.z);
        break;
      }
      Selected += check_step;
    }
    return;
  }

  //## Функция, вызываемая из цикла окна для рендера сцены
  void space::draw(const evInput & ev)
  {
  // Матрицу модели в расчетах не используем, так как
  // она единичная и на положение элементов влияние не оказывает

    calc_position(ev);
    recalc_borders();
    if (!CashedSnips.empty()) clear_cashed_snips();

    Prog3d.use();   // включить шейдерную программу
    Prog3d.set_uniform("mvp", tr::MatProjection * MatView);
    //prog3d.set_uniform("Selected", Selected);
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

} // namespace tr
