//============================================================================
//
// file: glsl.cpp
//
// GLSL shaders program class
//
//============================================================================

#include "glsl.hpp"

namespace tr {

///
/// \brief glsl::init
///
glsl::glsl(const std::list<std::pair<GLenum, std::string>>& L)
{
  id = glCreateProgram();
  CHECK_OPENGL_ERRORS
  for(auto& P: L) attach_shader(P.first, P.second);
}


///
/// \brief glsl::~glsl
///
glsl::~glsl(void)
{
  if(!is_linked) return;
  if(use_on) unuse();

  for(auto shader_id: Shaders)
  {
    glDetachShader(id, shader_id);
    glDeleteShader(shader_id);
  }
  glDeleteProgram(id);

#ifndef NDEBUG
  CHECK_OPENGL_ERRORS
#endif
}


////////
// Идентификатор ш/программы
//
GLuint glsl::get_id() { return id;}


////////
// Проверка корректности GLSL программы
//
void glsl::validate(void)
{
  glValidateProgram(id);
  GLint i;
  glGetProgramiv(id, GL_VALIDATE_STATUS, &i);
  if(false == i)
  {
    GLsizei log_length = 0;
    GLchar message[1024];
    glGetProgramInfoLog(id, 1024, &log_length, message);
    std::string msg = "Invalid GLSL program.\n";
    msg += message;
    ERR(msg);
  }
}


////////
// Линковка программы
//
void glsl::link()
{
  if(true == is_linked) return;

  glGetProgramiv(id, GL_LINK_STATUS, &is_linked);
  // Если уже была слинкована внешней процедурой, то просто выходим
  if(true == is_linked) return;
  // ,иначе линкуем на месте
  glLinkProgram(id);
  glGetProgramiv(id, GL_LINK_STATUS, &is_linked);
  // и проверяем результат
  if(false == is_linked)
  {
    GLsizei log_length = 0;
    GLchar message[1024];
    glGetProgramInfoLog(id, 1024, &log_length, message);
    std::string msg = "Can not link GLSL program.\n";
    msg += message;
    ERR(msg);
  }
  return;
}


////////
// Отключение программы GLSL
//
void glsl::unuse(void)
{
  if(!use_on){
    std::cerr << std::endl << __PRETTY_FUNCTION__
              << ": status use_on = false" << std::endl;
    return;
  }

  glUseProgram(0);
  use_on = false;

#ifndef NDEBUG
  CHECK_OPENGL_ERRORS
#endif

  return;
}


////////
// Подключение программы GLSL
//
void glsl::use(void)
{
  if(use_on){
    std::cerr << std::endl << __PRETTY_FUNCTION__
              << ": status use_on = true" << std::endl;
    return;
  }

  if(false == is_linked) link();
  glUseProgram(id);
  use_on = true;
  return;
}


////////
// Получение индекса атрибута шейдера по имени
//
GLuint glsl::attrib(const char * name )
{
  if(!is_linked)
  {
    std::string msg = "Can't get attrib \"";
    msg += name;
    msg += "\" - GLSL program not yet linked";
    ERR(msg);
  }

  GLint l = glGetAttribLocation(id, name);
  if(0 > l)
  {
    std::string msg = "GLSL: not found attrib name: ";
    msg += name;
    ERR(msg);
  }
  return static_cast<GLuint>(l);
}


////////
// Получение индекса переменной uniform по имени
//
GLint glsl::uniform(const char * name )
{
  if(!is_linked)
  {
    std::string msg = "Can't get uniform \"";
    msg += name;
    msg += "\" - program not yet linked";
    ERR(msg);
  }

  GLint l = glGetUniformLocation(id, name);
  if(0 > l)
  {
    std::string msg = "Not found uniform name: ";
    msg += name;
    ERR(msg);
  }

  return l;
}


////////
// Передача в шейдер матрицы
//
void glsl::set_uniform(const char * name, const glm::mat4 & m)
{
  GLint loc;
  loc = uniform(name);
  glUniformMatrix4fv(loc, 1, GL_FALSE, &m[0][0]);
  return;
}


////////
// Передача в шейдер вектора
//
void glsl::set_uniform(const char* name, const glm::vec3& m)
{
  GLint loc;
  loc = uniform(name);
  glUniform3fv(loc, 1, glm::value_ptr(m));
  return;
}

////////
// Передача в шейдер вектора
//
void glsl::set_uniform(const char * name, const glm::vec4 & m)
{
  GLint loc;
  loc = uniform(name);
  glUniform4fv(loc, 1, glm::value_ptr(m));
  return;
}


////////
// Передача в шейдер целого числа
//
void glsl::set_uniform(const char * name, GLint u)
{
  GLint loc;
  loc = uniform(name);
  glUniform1i(loc, u);
  return;
}


////////
// Передача в шейдер целого числа
//
void glsl::set_uniform1ui(const char* name, GLuint u)
{
  GLint loc = uniform(name);
  glUniform1ui(loc, u);
  return;
}


////////
// Передача в шейдер значения float
//
void glsl::set_uniform(const char * name, GLfloat u)
{
  GLint loc;
  loc = uniform(name);
  glUniform1f(loc, u);
  return;
}


///
/// \brief glsl::attach_shader
/// \param type - GL_VERTEX_SHADER | GL_FRAGMENT_SHADER | GL_GEOMETRY_SHADER | GL_TESS_CONTROL_SHADER | GL_TESS_EVALUATION_SHADER
/// \param fname
/// \details
///
///
void glsl::attach_shader(GLenum type, const std::string &fname)
{

    GLuint shader_id = glCreateShader(type);
    Shaders.push_back(shader_id);

#ifndef NDEBUG
    CHECK_OPENGL_ERRORS
#endif

  auto content = read_chars_file(fname);
  if(content == nullptr) ERR("Can't read file with shader content" + fname);

  char* ch = content.get();
  glShaderSource(shader_id, 1, &ch, nullptr);
  glCompileShader(shader_id);

  // проверка результата
  GLint _compiled;
  glGetShaderiv(shader_id, GL_COMPILE_STATUS, &_compiled);
  if (!_compiled)
  {
    GLsizei log_length = 0;
    GLchar message[1024];
    glGetShaderInfoLog(shader_id, 1024, &log_length, message);
    std::cerr << message;
    ERR("Error compile " + fname);
  }
  glAttachShader(id, shader_id);

  return;
}

} // namespace tr
