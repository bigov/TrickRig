/*
 * file: db.hpp
 *
 * Заголовочный файл класса управления базой данных
 *
 */

#ifndef DB_HPP
#define DB_HPP

#include "wsql.hpp"
#include "vox.hpp"
#include "glsl.hpp"
#include "trgl.hpp"

namespace tr {

// Структура для работы с воксом
struct vox_data
{
    int y = 0;                       // Y-координата вокса

    // Данные каждой стороны записываются в массив std::array<unsigned char, bytes_per_side + 1>
    // В котором в нулевой позиции записывается id стороны (Xp|Xn|Yp|Yn|Zp|Zn),
    // а за ней (в бинарном виде) данные для построения вершин в OpenGL.
    std::vector<face_t> Faces {};
};

// Структура для передачи данных группы воксов (столбец Y)
struct data_pack
{
    int x = 0;
    int z = 0;                      // координаты ячейки
    std::vector<vox_data> Voxes {}; // Массив воксов, хранящихся в БД
};
using vox_t = std::vector<vox_data>;


class db
{
  public:
    db(void) {}
   ~db(void) {}
    v_str open_app(const std::string &); // загрузка данных приложения
    v_str map_open(const std::string &); // загрузка данных карты
    void map_close(std::shared_ptr<glm::vec3> ViewFrom, float* look_dir);
    void map_name_save(const std::string &Dir, const std::string &MapName);
    v_ch map_name_read(const std::string & dbFile);
    void save_window_layout(const layout&);
    void init_map_config(const std::string &);
    data_pack area_load(int x, int z, int len);
    data_pack load_data(const int x, const int z);
    std::vector<uchar> blob_make(data_pack& DataPack);
    void vox_delete(const int x, const int y, const int z, const int len);
    void vox_append(const int x, const int y, const int z, const int len);

  private:
    std::string MapDir       {}; // директория текущей карты (со слэшем в конце)
    std::string MapPFName    {}; // имя файла карты
    std::string CfgMapPFName {}; // файл конфигурации карты/вида
    std::string CfgAppPFName {}; // файл глобальных настроек приложения
    wsql SqlDb {};

    void init_app_config(const std::string &);
    v_str load_config(size_t params_count, const std::string &FilePath);
    void update_row(const std::vector<uchar>& BlobData, int x, int z);
    void vox_face_erase(vox_data& VoxData, unsigned char face_id);
    vox_t::iterator vox_find(vox_t& DataPack, const int y);
};

} //tr
#endif
