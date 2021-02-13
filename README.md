# TRICKRIG
## минималистичный 3D движок


<a title="CMake status" href="https://github.com/bigov/trickrig/actions?query=workflow%3ACMake"><img alt="CMake workflow Status" src="https://github.com/bigov/trickrig/workflows/CMake/badge.svg"></a>

![CMake](https://github.com/bigov/trickrig/workflows/CMake/badge.svg)

Trickrig - это разрабатываемое на C++ ядро минималистичного графического 3D движка на основе _OpenGL_, с ипользованием свободных библиотек _glfw3, libpng16, sqlite3, glm_. В движке реализована многопоточность, минималистичное меню не привязанное к внешним библиотекам, заложена возможность реализации LOD.
 
Разрабатываемый код является мультиплатфоменным. Сборка на платформе _MS-Windows_ не требует наличия "Visual Studio", а производится с использованием открытых инструментов MSYS2. На платформе _Linux_ сборки выполняется с использованием "стандартных" средств разработки.

![demo](demo0.png)

TrickRig при перемещении камеры обеспечивает динамическое перестроение данных OpenGL VAO при рендере активной сцены, что обеспечивает эффективное использование графической памяти приложения, и позволяет генерировать "бесконечные" открытые 3D пространства.

Подробности на сайте [www.trickrig.net](https://www.trickrig.net)
