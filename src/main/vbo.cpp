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


vbo::vbo(GLenum type)
{
  gl_buffer_type = type;
  glGenBuffers(1, &id);
}


vbo::vbo(GLenum type, GLuint _id)
{
  gl_buffer_type = type;
  id = _id;
}


///
/// \brief vbo::allocate
/// \param al
/// \param data
///
/// \details Настройка размера буфера и заполнение его данными
///
void vbo::allocate(const GLsizeiptr new_size, const GLvoid* data)
{
#ifndef NDEBUG //--контроль создания буфера--------------------------------
  assert(id > 0 && "Ошибка: VBO буфер не инициализирован");
#endif
  //if(0 == id) glGenBuffers(1, &id);

  hem = 0;
  allocated = new_size;
  glBindBuffer(gl_buffer_type, id);
  glBufferData(gl_buffer_type, new_size, data, GL_STATIC_DRAW);

  #ifndef NDEBUG //--контроль создания буфера--------------------------------
  GLint d_size = 0;
  glGetBufferParameteriv(gl_buffer_type, GL_BUFFER_SIZE, &d_size);
  assert(new_size == d_size);
  #endif //------------------------------------------------------------------

  if(GL_ARRAY_BUFFER == gl_buffer_type) glBindBuffer(GL_ARRAY_BUFFER, 0);
  //glBindBuffer(gl_buffer_type, 0);
  if(nullptr != data) hem = new_size;
}


///
/// \brief vbo::attrib
/// \param index
/// \param d_size
/// \param type
/// \param normalized
/// \param stride
/// \param pointer
///
/// \details Настройка атрибутов для float
///
void vbo::set_attrib(GLuint index, GLint d_size, GLenum type, GLboolean normalized,
                 GLsizei stride, size_t pointer)
{
  glEnableVertexAttribArray(index);
  glBindBuffer(gl_buffer_type, id);
  glVertexAttribPointer(index, d_size, type, normalized, stride, reinterpret_cast<void*>(pointer));
  glBindBuffer(gl_buffer_type, 0);
}


///
/// \brief vbo::append
/// \param d_size
/// \param data
/// \return
///
/// \details Добавление данных в конец VBO (с контролем границы размера
/// буфера). Вносит данные в буфер по указателю границы данных блока (hem),
/// возвращает положение указателя по которому разместились данные и
/// сдвигает границу указателя на размер внесенных данных для приема
/// следующеего блока данных
///
GLsizeiptr vbo::append(const GLsizeiptr data_size, const GLvoid* data)
{
#ifndef NDEBUG
  if(nullptr == data) ERR("VBO::SubDataAppend nullptr");
  if((allocated - hem) < data_size) ERR("VBO::SubDataAppend got overflow buffer");
#endif

glBindBuffer(gl_buffer_type, id);
glBufferSubData(gl_buffer_type, hem, data_size, data);
glBindBuffer(gl_buffer_type, 0);
GLsizeiptr res = hem;
hem += data_size;
return res;
}


///
/// \brief vbo::remove
/// \param data_size
/// \param dest
/// \return
/// \details Для удаления данных из VBO используется перемещение: на место
/// удаляемого блока перемещаются данные из конца буфера, заменяя их. Длина
/// активной части буфера уменьшается на размер удаленного блока.
///
/// Возвращается значение границы VBO после переноса данных. Она равна
/// началу адреса блока, данные которого были перемещены.

GLsizeiptr vbo::remove(const GLsizeiptr data_size, const GLsizeiptr dest)
{
  if(data_size >= hem) hem = 0;
  if(hem == 0) return hem;

#ifndef NDEBUG
  if((dest + data_size) > hem)
  {
    std::printf("vbo_ext::remove error: dest %li + data_size %li  > hem %li\n",
                static_cast<long>(dest), static_cast<long>(data_size), static_cast<long>(hem));
    return hem;
  }
#endif
  auto src = hem - data_size; // Адрес крайнего на хвосте блока данных, которые будут перемещены.

  if(src != dest)
  {
    glBindBuffer(gl_buffer_type, id);
    glCopyBufferSubData(gl_buffer_type, gl_buffer_type, src, dest, data_size);
    glBindBuffer(gl_buffer_type, 0);
  }
  hem = src;
  return hem;
}


///
/// \brief vbo::update
/// \param data_size
/// \param data
/// \param stride
///
void vbo::update(const GLsizeiptr data_size, const GLvoid *data, GLsizeiptr stride)
{
  #ifndef NDEBUG
  if((stride + data_size) > allocated) ERR("vbo::update overflow the buffer");
  if((stride + data_size) > hem) std::cerr << "vbo::update overflow the current hem buffer";
  #endif

  glBindBuffer(gl_buffer_type, id);
  glBufferSubData(gl_buffer_type, stride, data_size, data);
  glBindBuffer(gl_buffer_type, 0);
}



// -------------------------- vbo_transit ----------------------------------------------
/*
///
/// \brief vbo_transit::vbo_ext
/// \param type
/// \details Если читать из рендер-буфера напрямую в ОЗУ, то после первого-же
/// считывания OpenGL автоматически устанавливает скорость операции перемещения
/// данных внутри памяти GPU равной скорости обмена с CPU, которая в разы ниже,
/// что сразу становится заметно визуально. Чтобы это обойти, создается
/// промежуточный (id_subbuf) буфер, через который производится чтение данных.
///
vbo_transit::vbo_transit(GLenum type): vbo(type)
{
  // Создание промежуточного буфера, через который будет
  // производится обмен данными между GPU и CPU.
  glGenBuffers(1, &id_subbuf);
  glBindBuffer(GL_COPY_WRITE_BUFFER, id_subbuf);
  glBufferData(GL_COPY_WRITE_BUFFER, bytes_per_face, nullptr, GL_STATIC_DRAW);
  glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
}


///
/// \brief vbo_transit::data_get
/// \param offset
/// \param size
/// \param data
///
void vbo_transit::data_get(GLintptr offset, GLsizeiptr sz, GLvoid* dst)
{
#ifndef NDEBUG
  if(sz > bytes_per_face) ERR("vbo_ext::data_get > bytes_per_snip");
#endif

  glBindBuffer(GL_COPY_WRITE_BUFFER, id_subbuf);
  glBindBuffer(GL_COPY_READ_BUFFER, id);
  glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, offset, 0, sz);
  glBindBuffer(GL_COPY_READ_BUFFER, 0);
  glBindBuffer(GL_COPY_WRITE_BUFFER, 0);

  glBindBuffer(GL_COPY_READ_BUFFER, id_subbuf);
  //glGetBufferSubData(GL_COPY_WRITE_BUFFER, offset, sz, dst);
  GLvoid* ptr = glMapBufferRange(GL_COPY_READ_BUFFER, 0, sz, GL_MAP_READ_BIT);
  memcpy(dst, ptr, size_t(sz));
  glUnmapBuffer(GL_COPY_READ_BUFFER);
  glBindBuffer(GL_COPY_READ_BUFFER, 0);

  if(glGetError() != GL_NO_ERROR) std::cerr << "Error in vbo_ext::data_get";
}
*/

} //tr
