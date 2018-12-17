/*
 * file: db.hpp
 *
 * Заголовочный файл класса управления базой данных
 *
 */

#ifndef DB_HPP
#define DB_HPP

#include "wsql.hpp"

namespace tr {

class db
{
  public:
    db(void) {}
   ~db(void) {}
    v_str open_map(const std::string &); // загрузка данных карты
    v_str open_app(const std::string &); // загрузка данных приложения
    static void save_map_name(const std::string &);
    static void save(const tr::camera_3d &Eye);
    static void save(const tr::main_window &AppWin);
    v_ch get_map_name(const std::string & dbFile);

  private:
    static std::string MapDir;
    static std::string MapPFName;
    static std::string CfgMapPFName;
    static std::string CfgAppPFName;
    static wsql SqlDb;

    void init_map_config(const std::string &);
    void init_app_config(const std::string &);
    v_str load_config(size_t params_count, const std::string &file_name);
};

} //tr
#endif
