# gditools3
![gditools3](screenshots/screenshot001win_small.png)

gditools3 is a fork of of https://sourceforge.net/projects/dcisotools/

This Python program/library is designed to handle GD-ROM image (GDI) files. It can be used to list files, extract data, generate sorttxt file, extract bootstrap (IP.BIN) file and more.

This project can be used in standalone mode, in interactive mode or as a library in another Python program (check the 'addons' folder to learn how).

You can use either the command-line interface (gditools3.py, for Python 2 or 3) or the GUI (gditools3gui.py, for Python 3 only).

This project was (barely!) tested with Python 2.7, 3.4 and 3.7.

See the README.TXT file for more informations.

Features
 - List and extract files from a GDI dump of a Dreamcast Gigabyte disc (GD-ROM)
 - Extract the bootsector (IP.BIN) of a GDI dump
 - Generate a sorttxt.txt of the files in a GDI dump
 - Transparent support for 2048 (iso) or 2352 (bin) data track format
 - Preserve the timestamp of extracted files
 - Usable as a library providing other programs with GDI-related functions
 - Python GUI (courtesy of [PySimpleGUI](https://github.com/MikeTheWatchGuy/PySimpleGUI)) with preview of text and image files and media playback via external player

## fork分支修改说明

原程序读取`.gdi`光碟索引文件有逻辑问题，对[Redump](http://redump.org/downloads/)版`.gdi`文件灾难性地无法读取。同时基础Py语法并不便于处理这类文件(怎么Py不自带个`scanf`、`fscanf`这样的函数呢……)，故只得额外编写了一个纯C小工具，**将`gdi`文件转换至中间态的`json`文件。使用主工具时应转为读入`json`文件。**

小工具`gdi2json`依赖`libcjson`、同时使用了`argp.h`、`error.h`两个`GNU/Linux GLibC`提供的头文件，`MinGW`没有提供这些……

为兼容原设计与`Redump`版`.gdi`文件，对轨道的读取逻辑设计为：

1. `"%u%u%u%u%s%u"`
2. `%u%u%u%u"一对半角英语双引号内的所有字，当然这串字里不该包含这个字符"%u`

上述修改均于`Ubuntu 26.04` `Python 3.14`下进行，故只可确保于此环境中工作基本无恙。
