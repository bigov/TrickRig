//============================================================================
//
// file: glsl.cpp
//
// GLSL shaders program class
//
//============================================================================

#include "glsl.hpp"

namespace tr {

  ////////
  // Конструктор шейдерной программы
  //
  glsl::glsl()
  {
    Atrib.clear();
  }


  ///
  /// \brief glsl::init
  ///
  void glsl::init(void)
  {
    if( id > 0 ) return;

    id = glCreateProgram();
    if (glGetError()) ERR("Can't create GLSL program");
  }


  ///
  /// \brief glsl::destroy
  ///
  void glsl::destroy(void)
  {
    unuse();
    for(auto shader_id: Shaders)
    {
      glDetachShader(id, shader_id);
      glDeleteShader(shader_id);
    }
    glDeleteProgram(id);
    id = 0;
  }


  ///
  /// \brief glsl::~glsl
  ///
  glsl::~glsl(void)
  {
    if(id != 0) info("ALERT: the glsl::destroy() method was missing!");
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
    if(true == isLinked) return;
  
    glGetProgramiv(id, GL_LINK_STATUS, &isLinked);
    // Если уже была слинкована внешней процедурой, то просто выходим
    if(true == isLinked) return;
    // ,иначе линкуем на месте
    glLinkProgram(id);
    glGetProgramiv(id, GL_LINK_STATUS, &isLinked);
    // и проверяем результат
    if(false == isLinked)
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
    glUseProgram(0);
#ifndef NDEBUG
    GLenum err = glGetError();
    switch (err)
    {
      case GL_NO_ERROR:
        break;
      case GL_INVALID_VALUE:
        info("program is neither 0 nor a value generated by OpenGL.");
        break;
      case GL_INVALID_OPERATION:
        info("program is not a program object, or could not be made part of current state, or transform feedback mode is active");
        break;
      default:
        info("Undefined error");
    }
#endif

    return;
  }
  
  ////////
  // Подключение программы GLSL
  //
  void glsl::use(void)
  {
    if(false == isLinked) link();
    glUseProgram(id);
    return;
  }
  
  ////////
  // Получение индекса атрибута шейдера по имени
  //
  GLuint glsl::attrib_location_get(const char * name )
  {
    if(!isLinked)
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

    auto result = static_cast<GLuint>( l );
    Atrib[name] = result;
    return result;
  }
  
  ////////
  // Получение индекса переменной uniform по имени
  //
  GLint glsl::uniform_location_get(const char * name )
  {
    if(!isLinked)
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
    loc = uniform_location_get(name);
    glUniformMatrix4fv(loc, 1, GL_FALSE, &m[0][0]);
    return;
  }
  
  ////////
  // Передача в шейдер вектора
  //
  void glsl::set_uniform(const char * name, const glm::vec3 & m)
  {
    GLint loc;
    loc = uniform_location_get(name);
    glUniform3fv(loc, 1, glm::value_ptr(m));
    return;
  }
  
  ////////
  // Передача в шейдер вектора
  //
  void glsl::set_uniform(const char * name, const glm::vec4 & m)
  {
    GLint loc;
    loc = uniform_location_get(name);
    glUniform4fv(loc, 1, glm::value_ptr(m));
    return;
  }


  ////////
  // Передача в шейдер целого числа
  //
  void glsl::set_uniform(const char * name, GLint u)
  {
    GLint loc;
    loc = uniform_location_get(name);
    glUniform1i(loc, u);
    return;
  }
  
  
  ////////
  // Передача в шейдер целого числа
  //
  void glsl::set_uniform1ui(const char* name, GLuint u)
  {
    GLint loc = uniform_location_get(name);
    glUniform1ui(loc, u);
    return;
  }


  ////////
  // Передача в шейдер значения float
  //
  void glsl::set_uniform(const char * name, GLfloat u)
  {
    GLint loc;
    loc = uniform_location_get(name);
    glUniform1f(loc, u);
    return;
  }
  
  ////////
  // Считывание из файла и подключение шейдера
  //
  void glsl::attach_shader(GLenum type, const std::string &fname)
  {
#ifndef NDEBUG
    if(id == 0) ERR("Need call glsl::init()");
#endif

    GLuint shader_id = glCreateShader(type);
    Shaders.push_back(shader_id);
    if(glGetError()) ERR("Can't glCreateShader");

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
      tr::info(message);
      ERR("Error compile " + fname);
    }
    glAttachShader(id, shader_id);
  
    return;
  }
  
  //## Считывание из файлов и подключение шейдеров
  //
  void glsl::attach_shaders(
      const std::string & vert_fn,
      const std::string & frag_fn
    )
  {
    attach_shader(GL_VERTEX_SHADER,   vert_fn);
    attach_shader(GL_FRAGMENT_SHADER, frag_fn);
    return;
  }
  
/*
  //## Считывание из файлов и подключение шейдеров
  //
  void glsl::attach_shaders(
      const std::string & vert_fn,
      const std::string & geom_fn,
      const std::string & frag_fn
    )
  {
    attach_shader(GL_VERTEX_SHADER,   vert_fn);
    attach_shader(GL_GEOMETRY_SHADER, geom_fn);
    attach_shader(GL_FRAGMENT_SHADER, frag_fn);
    return;
  }
*/

} // namespace tr
