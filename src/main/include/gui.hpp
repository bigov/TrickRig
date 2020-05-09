#ifndef GUI_HPP
#define GUI_HPP

#include "main.hpp"
#include "tools.hpp"

namespace tr
{
  enum HALIGN { LEFT, CENTER, RIGHT };
  enum VALIGN { TOP, MIDDLE, BOTTOM};
  enum STATE { ST_NORMAL, ST_OVER, ST_PRESSED, ST_INACTIVE };


  class element
  {
    public:
      element(void) = default;
      element(unsigned int width, unsigned int height);
      ~element(void) = default;

      void draw (STATE new_state = ST_NORMAL);

      void resize (unsigned int new_width, unsigned int new_height);
      uchar* data(void);

    protected:
      px BgColor { 0xFF, 0xFF, 0xFF, 0xFF };
      unsigned int width = 0;
      unsigned int height = 0;
      STATE state = ST_NORMAL;
      std::vector<uchar> Data {};
  };


  class label: public element
  {
    public:
      label(const std::string& new_text);

    protected:
      unsigned int font_id = 0;
      unsigned int font_height = 0;

      std::string text {};

      HALIGN text_halign = CENTER;
      VALIGN text_valign = MIDDLE;
      px text_color { 0x00, 0x00, 0x00, 0xFF };
      unsigned int text_width = 0;
  };
}

#endif // GUI_HPP
