/*
 * filename: framebuf.cpp
 *
 * Класс управления фрейм-буфером
 *
 *  glActiveTexture(GL_TEXTURE1); // текстура для рендера 3D пространства
 *  glActiveTexture(GL_TEXTURE3); // текстура идентификации примитивов
 *
 */

#include "framebuf.hpp"

namespace tr
{
///
/// \brief gl_texture::gl_texture
/// \details
///
/// void glTexImage2D(GLenum target, GLint level, GLint internalformat,
///                   GLsizei width, GLsizei height, GLint border,
///                   GLenum format, GLenum type, const GLvoid * data);
///
gl_texture::gl_texture(GLenum texture_num, GLint internalformat, GLenum format, GLenum type,
                       GLsizei width, GLsizei height, const GLvoid* data)
  : texture_num(texture_num), internalformat(internalformat), format(format), type(type)
{
  glActiveTexture(texture_num);
  glGenTextures(1, &texture_id);
  glBindTexture(target, texture_id);
  glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(target, level, internalformat, width, height, border, format, type, data);
}


///
/// \brief gl_texture::resize
/// \param width
/// \param height
/// \param data
/// \details Изменение размера памяти под текстуру. Инициалиированную область
/// можно заполнить изображением в формате RGBA - адрес данных изображения надо
/// передать в третьем параметре. Иначе область заливается ровным голубым цветом.
///
void gl_texture::resize(GLsizei width, GLsizei height, const GLvoid* data)
{
  bind();

  if(data == nullptr)
  {
    //image Blue{static_cast<ulong>(width), static_cast<ulong>(height),
    //  {0x7F, 0xB0, 0xFF, 0xFF}};  // голубой цвет (формат RGBA)
    //glTexImage2D(target, level, internalformat, width, height, border, format, type, Blue.uchar_t());

    std::vector<uchar> White (static_cast<uint>(width) * static_cast<uint>(height) * 4, 0xFF);
    glTexImage2D(target, level, internalformat, width, height, border, format, type, White.data());
  } else {
    glTexImage2D(target, level, internalformat, width, height, border, format, type, data);
  }

  unbind();
}


///
/// \brief gl_texture::bind
///
void gl_texture::bind(void)
{
  glActiveTexture(texture_num);
  glBindTexture(target, texture_id);
}


///
/// \brief gl_texture::unbind
///
void gl_texture::unbind(void)
{
  glActiveTexture(texture_num);
  glBindTexture(target, 0);
}


///
/// \brief gl_texture::id
/// \return
///
GLuint gl_texture::id(void) const
{
  return texture_id;
}


///
/// \brief frame_buffer::frame_buffer
/// \param w
/// \param h
///
frame_buffer::frame_buffer(GLsizei w, GLsizei h)
{
  glGenFramebuffers(1, &id);
  glBindFramebuffer(GL_FRAMEBUFFER, id);

  TexColor = std::make_unique<gl_texture>(GL_TEXTURE1, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, TexColor->id(), 0);

  TexIdent = std::make_unique<gl_texture>(GL_TEXTURE3, GL_R32I, ident_format, ident_type);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, TexIdent->id(), 0);

  GLenum b[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
  glDrawBuffers(2, b);

  glGenRenderbuffers(1, &rbuf_id);             // рендер-буфер (глубина и стенсил)
  glBindRenderbuffer(GL_RENDERBUFFER, rbuf_id);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbuf_id);

  glBindRenderbuffer(GL_RENDERBUFFER, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  resize(w, h);

#ifndef NDEBUG
  CHECK_OPENGL_ERRORS
#endif
  if(glGetError() != GL_NO_ERROR) ERR("Failure: can't init frame buffer.");
}


///
/// \brief fb_ren::resize
/// \param width
/// \param height
///
void frame_buffer::resize(int w, int h)
{
  glViewport(0, 0, w, h); // пересчет Viewport

  TexIdent->resize(w, h); // Текстура индентификации примитивов (канал RED)
  TexColor->resize(w, h);

  // настройка размера рендербуфера
  glBindRenderbuffer(GL_RENDERBUFFER, rbuf_id);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
  glBindRenderbuffer(GL_RENDERBUFFER, 0);
}


///
/// \brief fb_ren::bind
///
void frame_buffer::bind(void)
{
  glBindFramebuffer(GL_FRAMEBUFFER, id);
  glBindRenderbuffer(GL_RENDERBUFFER, rbuf_id);
  TexIdent->bind();
  TexColor->bind();
}


///
/// \brief framebuf::read_pixel
/// \param x
/// \param y
/// \return
///
void frame_buffer::read_pixel(GLint x, GLint y, void* pixel_data)
{
  assert((x >= 0) && (y >= 0));

  glBindFramebuffer(GL_READ_FRAMEBUFFER, id);
  glReadBuffer(GL_COLOR_ATTACHMENT1);
  glReadPixels(x, y, 1, 1, ident_format, ident_type, pixel_data);
  glReadBuffer(GL_NONE);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}


///
/// \brief fb_ren::unbind
///
void frame_buffer::unbind(void)
{
  glBindRenderbuffer(GL_RENDERBUFFER, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  //TexIdent->unbind();
  //TexColor->unbind();
}


///
/// \brief fb_ren::~fb_ren
///
frame_buffer::~frame_buffer(void)
{
  unbind();
  TexIdent->unbind();
  TexColor->unbind();
  glDeleteFramebuffers(1, &id);
}


}
