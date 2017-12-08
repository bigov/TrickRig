//============================================================================
//
// file: vbo.cpp
//
// Class GLSL VBOs control
//
//============================================================================
#include "vbo.hpp"

namespace tr
{
  //## Настройка границы контроля данных буфера
  void VBO::Resize(GLsizeiptr new_hem)
  {
    if(new_hem == hem) return;
    if(new_hem < 0) ERR("VBO: negative value of new size");
    if(new_hem > allocated) ERR("VBO: overfolw value of new size");
    hem = new_hem;
    return;
  }

  //## Cоздание нового буфера указанного в параметре размера
  void VBO::Allocate(GLsizeiptr al)
  {
    if(0 != id) ERR("VBO::Allocate trying to re-init exist object.");
    allocated = al;
    glGenBuffers(1, &id);
    glBindBuffer(gl_buffer_type, id);
    glBufferData(gl_buffer_type, allocated, 0, GL_STATIC_DRAW);

    #ifndef NDEBUG //--контроль создания буфера--------------------------------
    GLint d_size = 0;
    glGetBufferParameteriv(gl_buffer_type, GL_BUFFER_SIZE, &d_size);
    assert(allocated == d_size);
    #endif //------------------------------------------------------------------
    
    if(GL_ARRAY_BUFFER == gl_buffer_type) glBindBuffer(gl_buffer_type, 0);
    hem = 0;
    return;
  }

  //## Cоздание и заполнение буфера с указанным в параметрах данными
  void VBO::Allocate(GLsizeiptr al, const GLvoid* data)
  {
    if(0 != id) ERR("VBO::Allocate trying to re-init exist object.");
    allocated = al;
    glGenBuffers(1, &id);
    glBindBuffer(gl_buffer_type, id);
    glBufferData(gl_buffer_type, allocated, data, GL_STATIC_DRAW);

    #ifndef NDEBUG //--контроль создания буфера--------------------------------
    GLint d_size = 0;
    glGetBufferParameteriv(gl_buffer_type, GL_BUFFER_SIZE, &d_size);
    assert(allocated == d_size);
    #endif //------------------------------------------------------------------
    
    if(GL_ARRAY_BUFFER == gl_buffer_type) glBindBuffer(gl_buffer_type, 0);
    hem = al;
    return;
  }

  //## Настройка атрибутов для float
  void VBO::Attrib(GLuint index, GLint d_size, GLenum type, GLboolean normalized,
    GLsizei stride, const GLvoid* pointer)
  {
    glEnableVertexAttribArray(index);
    glBindBuffer(gl_buffer_type, id);
    glVertexAttribPointer(index, d_size, type, normalized, stride, pointer);
    glBindBuffer(gl_buffer_type, 0);
    return;
  }

  //## Настройка атрибутов для int
  void VBO::AttribI(GLuint index, GLint d_size, GLenum type,
    GLsizei stride, const GLvoid* pointer)
  {
    glEnableVertexAttribArray(index);
    glBindBuffer(gl_buffer_type, id);
    glVertexAttribIPointer(index, d_size, type, stride, pointer);
    glBindBuffer(gl_buffer_type, 0);
    return;
  }

  //## Сжатие буфера атрибутов за счет перемещения данных из хвоста
  // в неиспользуемый промежуток и уменьшение текущего индекса
  void VBO::Reduce(GLintptr src, GLintptr dst, GLsizeiptr d_size)
  {
    #ifndef NDEBUG // контроль точки переноса
      if(dst > (src - d_size)) ERR("VBO::Reduce got err dst");
    #endif

    glBindBuffer(gl_buffer_type, id);
    glCopyBufferSubData(gl_buffer_type, gl_buffer_type, src, dst, d_size);
    glBindBuffer(gl_buffer_type, 0);
    hem -= d_size;
    return;
  }

  //## Внесение данных с контролем границы максимально выделенного размера буфера
  GLsizeiptr VBO::SubDataAppend(GLsizeiptr d_size, const GLvoid* data)
  {
  /* вносим данные в буфер по указателю конца блока (size), возвращаем положение
   * указателя по которому разместили данные и увеличиваем значение указателя на
   * размер внесенных данных для последующего использования */

    #ifndef NDEBUG
      if((allocated - hem) < d_size) ERR("VBO::SubDataAppend got overflow buffer");
    #endif //------------------------------------------------------------------

    if(GL_ARRAY_BUFFER == gl_buffer_type) glBindBuffer(gl_buffer_type, id);
    glBufferSubData(gl_buffer_type, hem, d_size, data);
    if(GL_ARRAY_BUFFER == gl_buffer_type) glBindBuffer(gl_buffer_type, 0);
    GLsizeiptr res = hem;
    hem += d_size;
    return res;
  }

  //## Замена указанной группы атрибутов с контролем границы размера буфера
  void VBO::SubDataUpdate(GLsizeiptr d_size, const GLvoid* data, GLsizeiptr idx)
  {
    #ifndef NDEBUG //--проверка свободного места в буфере----------------------
    if (idx > (hem - d_size)) ERR("VBO::SubDataUpdate got bad index for update attrib group");
    if ((allocated - hem) < d_size) ERR("VBO::SubDataUpdate overflow buffer");
    #endif //------------------------------------------------------------------

    glBindBuffer(gl_buffer_type, id);
    glBufferSubData(gl_buffer_type, idx, d_size, data);
    glBindBuffer(gl_buffer_type, 0);
    return;
  }

} //tr
