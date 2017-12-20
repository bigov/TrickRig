//============================================================================
//
// file: scene.cpp
//
// Управление пространством сцены
//
//============================================================================
#include "scene.hpp"

namespace tr
{
  //## Конструктор
  //
  scene::scene()
  {

    program2d_init();
    framebuffer_init();

    // Загрузка символов для отображения fps
    ttf.init(tr::config::filepath(FONT_FNAME), 9);
    ttf.load_chars( L"fps: 0123456789" );
    ttf.set_cursor( 2, 1 );
    ttf.set_color( 0x18, 0x18, 0x18 );
    show_fps.w = 46;
    show_fps.h = 15;
    show_fps.size = static_cast<size_t>(show_fps.w * show_fps.h) * 4;
    show_fps.img.assign(
      show_fps.size, 0x00);

    // Загрузка обрамления окна (HUD) из файла
    pngImg image = get_png_img(tr::config::filepath(HUD_FNAME));

    glGenTextures(1, &text);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, text);

    GLint level_of_details = 0, frame = 0;
    
    glTexImage2D(GL_TEXTURE_2D, level_of_details, GL_RGBA,
      image.w, image.h, frame, GL_RGBA, GL_UNSIGNED_BYTE, image.img.data());

    //glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    // Clamping to edges is important to prevent artifacts when scaling
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Linear filtering usually looks best for text
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return;
  }

  //## Деструктор
  //
  scene::~scene(void)
  {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &frameBuffer);
    return;
  }

  //## Инициализация фрейм-буфера
  //
  void scene::framebuffer_init(void)
  {
    glGenFramebuffers(1, &frameBuffer);
    glActiveTexture(GL_TEXTURE0);
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

    glGenTextures(1, &texColorBuffer);
    glBindTexture(GL_TEXTURE_2D, texColorBuffer);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tr::Cfg.gui.w, tr::Cfg.gui.h, 0,
      GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glFramebufferTexture2D(
      GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texColorBuffer, 0
    );
    
    GLuint rb;
    glGenRenderbuffers(1, &rb);
    glBindRenderbuffer(GL_RENDERBUFFER, rb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8,
      tr::Cfg.gui.w, tr::Cfg.gui.h);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
      GL_RENDERBUFFER, rb);

    #ifndef NDEBUG //--контроль создания буфера-------------------------------
    assert(
      glCheckFramebufferStatus( GL_FRAMEBUFFER ) == GL_FRAMEBUFFER_COMPLETE
    );
    #endif

    return;
  }

  //## Инициализация GLSL программы обработки текстуры из фреймбуфера.
  //
  //
  void scene::program2d_init(void)
  {
    glGenVertexArrays(1, &vaoQuad);
    glBindVertexArray(vaoQuad);

    screenShaderProgram.attach_shaders(
      tr::config::filepath(SHADER_VERT_SCREEN),
      tr::config::filepath(SHADER_FRAG_SCREEN)
    );
    screenShaderProgram.use();

    tr::vbo vboPosition = {GL_ARRAY_BUFFER};
    GLfloat Position[] = { -1.f, -1.f, 1.f, -1.f, -1.f, 1.f, 1.f, 1.f };
    vboPosition.allocate( sizeof(Position), Position );
    vboPosition.attrib( screenShaderProgram.attrib_location_get("position"),
        2, GL_FLOAT, GL_FALSE, 0, nullptr );

    tr::vbo vboTexcoord = {GL_ARRAY_BUFFER};
    GLfloat Texcoord[] = { 0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 1.f, 1.f };
    vboTexcoord.allocate( sizeof(Texcoord), Texcoord );
    vboTexcoord.attrib( screenShaderProgram.attrib_location_get("texcoord"),
        2, GL_FLOAT, GL_FALSE, 0, nullptr );

    glUniform1i(screenShaderProgram.uniform_location_get("texFramebuffer"), 0);
    glUniform1i(screenShaderProgram.uniform_location_get("texHUD"), 1);
    screenShaderProgram.unuse();

    glBindVertexArray(0);

    return;
  }

  //## Рендеринг
  //
  // Кадр сцены рисуется в виде изображения на (2D) "холсте" фреймбуфера,
  // после чего это изображение в виде текстуры накладывается на прямоугольник
  // окна. Курсор и дополнительные (HUD) элементы сцены изображаются
  // аналогично - как наложеные сверху рисунки с (полу-)прозрачным фоном
  void scene::draw(const evInput& ev)
  {
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
    space.draw(ev);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glBindVertexArray(vaoQuad);
    glDisable(GL_DEPTH_TEST);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texColorBuffer);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, text);

    // Табличка с ткстом на экране отображается в виде наложенной картинки
    show_fps.img.clear();
    show_fps.img.assign(show_fps.size, 0xCD);
    ttf.set_cursor(2,1);
    ttf.write_wstring(show_fps, {L"fps:" + std::to_wstring(ev.fps)});

    glTexSubImage2D(GL_TEXTURE_2D, 0, 6, 6,
      show_fps.w, show_fps.h, GL_RGBA, GL_UNSIGNED_BYTE, show_fps.img.data());

    screenShaderProgram.use();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    screenShaderProgram.unuse();

    return;
  }

} // namespace tr

//Local Variables:
//tab-width:2
//End: