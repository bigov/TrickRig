#ifndef _MAP_HPP_
#define _MAP_HPP_

#include <iostream>
#include <vector>
#include <array>


namespace font {

struct sprite {
    const std::string S;
    unsigned char u;
    unsigned char v;
};

static const char texture_file[] = "texture.png";

static const int texture_cols = 16;
static const int texture_rows = 4;

static const float texture_w = 112.f;
static const float texture_h = 28.f;
static const float sym_w = 7.f;
static const float sym_h = 7.f;

static const float sym_u_size = sym_w / texture_w;
static const float sym_v_size= sym_h / texture_h;

static const std::vector<sprite> symbols_map = {
  {"0",0,0}, {"O",0,0}, {"О",0,0}, {"1",1,0}, {"2",2,0}, {"3",3,0},
  {"З",3,0}, {"з",3,0}, {"4",4,0}, {"Ч",4,0}, {"ч",4,0}, {"5",5,0},
  {"6",6,0}, {"7",7,0}, {"8",8,0}, {"9",9,0}, {"A",10,0}, {"a",10,0},
  {"а",10,0}, {"А",10,0}, {"B",11,0}, {"В",11,0}, {"в",11,0}, {"b",11,0},
  {"C",12,0}, {"С",12,0}, {"c",12,0}, {"с",12,0}, {"D",13,0}, {"d",13,0},
  {"E",14,0}, {"Е",14,0}, {"e",14,0}, {"е",14,0}, {"F",15,0}, {"f",15,0},
  {"G",0,1}, {"g",0,1}, {"H",1,1}, {"Н",1,1}, {"h",1,1}, {"н",1,1},
  {"I",2,1}, {"i",2,1}, {"J",3,1}, {"j",3,1}, {"K",4,1}, {"k",4,1},
  {"К",4,1}, {"к",4,1}, {"L",5,1}, {"l",5,1}, {"M",6,1}, {"М",6,1},
  {"m",6,1}, {"м",6,1}, {"N",7,1}, {"n",7,1}, {"P",8,1}, {"p",8,1},
  {"Р",8,1}, {"р",8,1}, {"Q",9,1}, {"q",9,1}, {"R",10,1}, {"r",10,1},
  {"S",11,1}, {"s",11,1}, {"T",12,1}, {"t",12,1}, {"Т",12,1}, {"т",12,1},
  {"U",13,1}, {"u",13,1}, {"V",14,1}, {"v",14,1}, {"W",15,1}, {"w",15,1},
  {"X",0,2}, {"x",0,2}, {"Х",0,2}, {"х",0,2}, {"Y",1,2}, {"y",1,2},
  {"У",1,2}, {"у",1,2}, {"Z",2,2}, {"z",2,2}, {"Б",3,2}, {"б",3,2},
  {"Г",4,2}, {"г",4,2}, {"Д",5,2}, {"д",5,2}, {"Ж",6,2}, {"ж",6,2},
  {"И",7,2}, {"Й",7,2}, {"и",7,2}, {"й",7,2}, {"Л",8,2}, {"л",8,2},
  {"П",9,2}, {"п",9,2}, {"Ф",10,2}, {"ф",10,2}, {"Ц",11,2}, {"ц",11,2},
  {"Ш",12,2}, {"ш",12,2}, {"Щ",13,2}, {"щ",13,2}, {"Ъ",14,2}, {"ъ",14,2},
  {"Ы",15,2}, {"ы",15,2}, {"Ь",0,3}, {"ь",0,3}, {"Э",1,3}, {"э",1,3},
  {"Ю",2,3}, {"ю",2,3}, {"Я",3,3}, {"я",3,3}, {".",4,3}, {",",5,3},
  {":",6,3}, {";",7,3}, {"+",8,3}, {"-",9,3}, {"/",10,3}, {"*",11,3},
  {"|",12,3}, {"(",12,3}, {")",12,3}, {"[",12,3}, {"]",12,3}, {"{",12,3},
  {"}",12,3}, {"!",13,3}, {"?",14,3}, {" ",15,3},
};


///
/// \brief string2vector
/// \param Text
/// \return
/// \details Конвертирует UTF-8 текстовую строку в массив (вектор) отдельных
/// символов, независимо от числа байт в символе.
///
std::vector<std::string> string2vector(const std::string& Text)
{
  std::vector<std::string> result {};
  for(size_t i = 0; i < Text.size();)
  {
    size_t bytes = 1;
    auto c = static_cast<unsigned char>(Text[i]);
    if( c >= 0xD0 ) bytes = 2;
    result.push_back(Text.substr(i, bytes));
    i += bytes;
  }
  return result;
}


std::array<float, 4> char_location(const std::string& Sym)
{
  unsigned int i;
  for(i = 0; i < symbols_map.size(); i++) if( symbols_map[i].S == Sym ) break;

  float u = static_cast<float>(symbols_map[i].u);
  float v = static_cast<float>(symbols_map[i].v);

  float u0 = sym_u_size * u,  v1 = sym_v_size * v;
  float u1 = sym_u_size + u0, v0 = sym_v_size + v1;

  std::array<float, 4> result { u0, v0, u1, v1 };
  return result;
}


std::array<unsigned int, 2> map_location(const std::string& Sym)
{
  unsigned int i;
  for(i = 0; i < symbols_map.size(); i++) if( symbols_map[i].S == Sym ) break;

  unsigned int u = symbols_map[i].u;
  unsigned int v = symbols_map[i].v;

  std::array<unsigned int, 2> result { u, v };
  return result;
}

} //namespace


#endif
