#ifndef WGLFW_HPP
#define WGLFW_HPP

#include "io.hpp"

namespace tr
{

class trgl
{
  public:
    trgl(const char* title = "\0");
    ~trgl(void);

    // Запретить копирование объекта
    trgl(const trgl&) = delete;
    trgl& operator=(const trgl&) = delete;

    // Запретить перенос объекта
    trgl(trgl&&) = delete;
    trgl& operator=(trgl&&) = delete;

    void thread_enable(void) { glfwMakeContextCurrent(win_thread); }
    void set_window(uint width=10, uint height=10, uint min_w=0,
                    uint min_h=0, uint left=0, uint top=0);
    void swap_buffers(void);
    void cursor_hide(void);
    void cursor_restore(void);
    void set_cursor_pos(double x, double y);
    void get_frame_buffer_size(int* width, int* height);
    GLFWwindow* get_id(void) const;

    void set_error_observer(interface_gl_context& ref);    // отслеживание ошибок
    void set_cursor_observer(interface_gl_context& ref);   // курсор мыши в окне
    void set_button_observer(interface_gl_context& ref);   // кнопки мыши
    void set_keyboard_observer(interface_gl_context& ref); // клавиши клавиатуры
    void set_position_observer(interface_gl_context& ref); // положение окна
    void add_size_observer(interface_gl_context& ref);     // размер окна
    void set_char_observer(interface_gl_context& ref);     // ввод текста (символ)
    void set_close_observer(interface_gl_context& ref);    // закрытие окна
    void set_focuslost_observer(interface_gl_context& ref);// смена фокуса

  private:
    GLFWwindow* win_main = nullptr;
    GLFWwindow* win_thread = nullptr;

    static interface_gl_context* error_observer;
    static interface_gl_context* cursor_observer;
    static interface_gl_context* button_observer;
    static interface_gl_context* keyboard_observer;
    static interface_gl_context* position_observer;
    static std::list<interface_gl_context*> size_observers;
    static interface_gl_context* char_observer;
    static interface_gl_context* close_observer;
    static interface_gl_context* focuslost_observer;

    static void callback_error(int error_id, const char* description);
    static void callback_cursor(GLFWwindow* window, double xpos, double ypos);
    static void callback_button(GLFWwindow* window, int button, int action, int mods);
    static void callback_keyboard(GLFWwindow*, int key, int scancode, int action, int mods);
    static void callback_position(GLFWwindow*, int, int);
    static void callback_size(GLFWwindow*, int, int);
    static void callback_char(GLFWwindow*, unsigned int);
    static void callback_close(GLFWwindow*);
    static void callback_focus(GLFWwindow*, int focused);
  };

} //namespace tr

#endif //WGLFW_HPP