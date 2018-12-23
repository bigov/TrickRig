/*
 * file: db.hpp
 *
 * Заголовочный файл класса управления базой данных
 *
 */

#ifndef DB_HPP
#define DB_HPP

#include "wsql.hpp"
#include "rig.hpp"
#include "rdb.hpp"

namespace tr {

class db
{
  public:
    db(void);
   ~db(void) {}
    v_str open_map(const std::string &); // загрузка данных карты
    v_str open_app(const std::string &); // загрузка данных приложения
    void map_name_save(const std::string &Dir, const std::string &MapName);
    v_ch map_name_read(const std::string & dbFile);
    void save(const camera_3d &Eye);
    void save(const main_window &AppWin);
    rig load_rig(const i3d &, const std::string &file_name);
    void rigs_loader(std::map<i3d, rig> &Map, i3d &Start, i3d &End);
    void save_rig(const i3d &, const rig *);
    void save_rigs_block(const i3d &, const i3d &, rdb &);
    void init_map_config(const std::string &);

  private:
    std::string MapDir       {}; // директория текущей карты (со слэшем в конце)
    std::string MapPFName    {}; // имя файла карты
    std::string CfgMapPFName {}; // файл конфигурации карты/вида
    std::string CfgAppPFName {}; // файл глобальных настроек приложения
    wsql SqlDb {};

    // == L-O-D 1 ==

    std::map<i3d, rig> TplRigs_1 {};     // Шаблон карты - для создания новых элементов.
    int tpl_1_side = 16;                 // длина стороны шаблона
    void load_template(int level = 1);   // загрузка шаблона из файла БД

    void init_app_config(const std::string &);
    v_str load_config(size_t params_count, const std::string &file_name);
};

} //tr
#endif
