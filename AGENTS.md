# AGENTS.md — руководство для агентов по работе с проектом UEFI-tools

## О проекте

Репозиторий `UEFI-tools` содержит форк утилиты [UEFITool](https://github.com/LongSoft/UEFITool) (ветка `new_engine`, версия `NE alpha 76`) и набор тестовых файлов BIOS. В форке реализован функционал редактирования UEFI-образов (insert/replace/remove/rebuild) в графической утилите и добавлена консольная утилита `UEFIEdit` для автоматизированного тестирования.

Полное описание реализации — в [`IMPLEMENTATION.md`](IMPLEMENTATION.md) в корне репозитория. Перед началом работы прочтите его.

## Структура репозитория

```
UEFI-tools/                      (корень git-репозитория = форк LongSoft/UEFITool)
├── AGENTS.md                    # этот файл
├── IMPLEMENTATION.md            # подробное описание реализации редактирования
├── README.md                    # оригинальный README UEFITool + дополнение про форк
├── LICENSE.md                   # BSD-2-Clause
├── .gitignore
├── version.h                    # версия программы
├── meson.build                  # корневой meson
├── CMakeLists.txt               # корневой cmake
├── fw/                          # тестовые файлы (НЕ модифицировать)
│   ├── HNX99TF_200525_original_E5C88C6F.bin    # 16 МБ образ BIOS (Intel flash)
│   ├── Mashinist_DXE_driver_SerialIo_SerialIo.ffs
│   └── MAshinist_DXE_driver_TerminalSrc_TerminalSrc.ffs
├── common/                      # общий движок (парсер, builder, ops, типы, сжатие)
├── UEFITool/                    # графическая утилита (Qt5)
├── UEFIExtract/                 # консольный дампер
├── UEFIFind/                    # консольный поиск
└── UEFIEdit/                    # консольный редактор (добавлен в форке)
```

### Каталог `common/` — общий движок

| Файл | Назначение |
|------|-----------|
| `basetypes.h` | Базовые типы (UINT8/16/32/64), USTATUS-коды, `EFI_GUID`, режимы (CREATE_MODE_*, EXTRACT_MODE_*, REPLACE_MODE_*) |
| `types.h` | `Actions::*` (NoAction/Insert/Rebuild/Remove/...), `Types::*` (Volume/File/Section/...), `Subtypes::*` |
| `ffs.h` | Структуры FFS: `EFI_FFS_FILE_HEADER`, `EFI_COMMON_SECTION_HEADER`, константы типов секций, GUID-ы |
| `ffs.cpp` | Реализация структур, `uint32ToUint24`, `uint24ToUint32`, `guidToUString` |
| `parsingdata.h` | `VOLUME_PARSING_DATA`, `FILE_PARSING_DATA`, `COMPRESSED_SECTION_PARSING_DATA` |
| `treemodel.h/cpp` | `TreeModel` (наследник `QAbstractItemModel` при QT_CORE_LIB, иначе своя реализация) |
| `treeitem.h/cpp` | `TreeItem` — элемент дерева |
| `ffsparser.h/cpp` | `FfsParser` — парсинг образа в дерево (6947 строк) |
| `ffsbuilder.h/cpp` | `FfsBuilder` — пересборка дерева в образ |
| `ffsops.h/cpp` | `FfsOperations` — extract/replace/remove/rebuild |
| `utility.h/cpp` | `calculateSum8`, `calculateChecksum8/16`, `decompress`, `errorCodeToUString` |
| `LZMA/`, `Tiano/`, `brotli/`, `zlib/` | Сжатие/декомпрессия (bundled) |
| `kaitai/`, `generated/`, `ksy/` | KaitaiStruct-парсеры NVRAM-хранилищ |

### Каталог `UEFITool/` — графическая утилита

| Файл | Назначение |
|------|-----------|
| `uefitool.h/cpp` | `UEFITool` (QMainWindow) — слоты insert/replace/remove/rebuild/saveImageFile |
| `uefitool.ui` | UI-форма (меню, контекстные меню, dock-виджеты) |
| `uefitool.pro` | qmake-проект |
| `uefitool_main.cpp` | `main()` + `UEFIToolApplication` |
| `QHexView/` | Hex-viewer (внешняя библиотека) |
| `searchdialog.*`, `hexviewdialog.*`, `goto*dialog.*` | Диалоги |

### Каталог `UEFIEdit/` — консольный редактор (новый)

| Файл | Назначение |
|------|-----------|
| `uefiedit.h/cpp` | Класс `UEFIEdit` |
| `uefiedit_main.cpp` | CLI-парсер |
| `uefiedit.pro` | qmake (без Qt) |
| `CMakeLists.txt` | cmake |
| `meson.build` | meson |

## Сборка

### Графическая утилита UEFITool (qmake, обязательно)

```bash
cd UEFITool
qmake-qt5 uefitool.pro
make -j$(nproc)
./UEFITool  # ELF 64-bit LSB executable, x86-64
```

После изменения исходников:
```bash
cd UEFITool
touch ../common/ffsbuilder.cpp ../common/ffsops.cpp ../common/treemodel.cpp
make -j$(nproc)
```

### Консольная утилита UEFIEdit

Три системы сборки — все эквивалентны:

```bash
# qmake (быстро, для разработки)
cd UEFIEdit
qmake-qt5 uefiedit.pro
make -j$(nproc)

# cmake
mkdir build && cd build && cmake ../UEFIEdit && make -j$(nproc)

# meson (собирает все утилиты)
meson setup build .
ninja -C build UEFIEdit/UEFIEdit
```

## Проверка (lint / typecheck)

Стандартные линтеры (ruff, cppcheck, clang-format) в проекте не настроены и не требуются. Используется только компилятор:

```bash
# Проверка компиляции GUI
cd UEFITool && make -j$(nproc) 2>&1 | grep -E "error:"

# Проверка компиляции UEFIEdit
cd UEFIEdit && make -j$(nproc) 2>&1 | grep -E "error:"

# Быстрый smoke-тест GUI (headless)
QT_QPA_PLATFORM=offscreen timeout 6 ./UEFITool/UEFITool fw/HNX99TF_200525_original_E5C88C6F.bin
# exit=0 — OK

# Smoke-тест UEFIEdit
./UEFIEdit/UEFIEdit fw/HNX99TF_200525_original_E5C88C6F.bin dump | head
./UEFIEdit/UEFIEdit fw/HNX99TF_200525_original_E5C88C6F.bin save /tmp/out.bin
```

Запускайте эти команды **после каждого изменения** в `common/` или `UEFITool/`/`UEFIEdit/`, чтобы убедиться, что сборка не сломалась.

## Соглашения по коду

- **C++11** (`CONFIG += c++11` в qmake, `-std=gnu++11` в gcc).
- **Без комментариев** в коде, если явно не запрошено. Оригинальный UEFITool следует BSD-стилю с copyright-заголовками в каждом файле — сохраняйте их при создании новых файлов.
- **Отступы**: 4 пробела (см. существующие файлы).
- **Скобки**: открывающая на той же строке для `else if`/`else`, на новой для функций/классов.
- **Имена**: `camelCase` для методов/переменных, `PascalCase` для классов, `UPPER_CASE` для констант/макросов.
- **Структуры FFS** (`EFI_FFS_FILE_HEADER`, `EFI_COMMON_SECTION_HEADER` и т.д.) — не модифицировать, они из спецификации UEFI PI.
- **UString/UByteArray** — абстракции над `QString`/`QByteArray` (при `QT_CORE_LIB`) или bstrlib (без Qt). Код в `common/` должен работать в обоих режимах.
- **`toLocal8Bit()`** возвращает `const char*` без Qt и `QByteArray` с Qt. В консольных утилитах (без Qt) можно писать `std::cerr << s.toLocal8Bit()`; в GUI-коде Qt — `.constData()` если нужен `const char*`.

## Ключевые архитектурные моменты

### TreeModel::addItem — критичная семантика аргумента `parent`

```cpp
UModelIndex TreeModel::addItem(offset, type, subtype, name, text, info,
                                header, body, tail, fixed,
                                const UModelIndex & parent, const UINT8 mode);
```

- `CREATE_MODE_APPEND`/`PREPEND`: `parent` = **контейнер**, новый элемент становится его ребёнком.
- `CREATE_MODE_BEFORE`/`AFTER`: `parent` = **reference-элемент**, `addItem` берёт `parent.internalPointer()` и вставляет новый элемент рядом с его родителем.

Это означает, что для insert-before/after нужно передавать **выбранный элемент**, а не его родителя. Подробно — в IMPLEMENTATION.md, баг 1.

### Отображаемое имя Volume — это не FileSystemGuid

Имя тома в GUI/UEFIEdit (`5C60F367-...`) берётся из `EFI_FIRMWARE_VOLUME_EXT_HEADER.FvName`, который хранится в `VOLUME_PARSING_DATA.extendedHeaderGuid`. `FileSystemGuid` из `EFI_FIRMWARE_VOLUME_HEADER` — это GUID файловой системы (`8C8CE578` для FFSv2, `5473C07A` для FFSv3), одинаковый для всех томов одной версии. При поиске тома по GUID используйте `parsingData`, а не `header`.

### Каскадная пометка для rebuild

Любое изменение (insert/remove/replace) требует пометки `Actions::Insert`/`Remove`/`Replace` для изменённого элемента **и** `Actions::Rebuild` для всех его предков до root. Без этого `FfsBuilder::build()` не пересоберёт образ. Подробно — в IMPLEMENTATION.md, баг 8.

```cpp
for (UModelIndex p = index.parent(); p.isValid() && model->type(p) != Types::Root; p = p.parent()) {
    if (model->action(p) == Actions::NoAction)
        model->setAction(p, Actions::Rebuild);
}
```

### FreeSpace в Volume

`buildVolume` откладывает построение `Types::FreeSpace` детей до конца, суммирует их размер, и после построения остальных детей проверяет, что превышение body помещается в freeSpace. Это позволяет вставлять/заменять файлы, если в томе есть свободное место. Не удаляйте эту логику.

### Контрольные суммы FFS

При любой модификации файла (`buildFile` с `Rebuild`/`Replace`/`Insert`) пересчитываются:
- **Header checksum**: `0x100 - (sum8(header) - Header - File - State)`.
- **Data checksum**: реальная если `FFS_ATTRIB_CHECKSUM`, иначе `FFS_FIXED_CHECKSUM` (0x5A для rev1) или `FFS_FIXED_CHECKSUM2` (0xAA для rev2).
- **Tail** (только rev1 + `FFS_ATTRIB_TAIL_PRESENT`): `~IntegrityCheck.TailReference`.

## Тестирование

### Тестовые файлы

- `fw/HNX99TF_200525_original_E5C88C6F.bin` — образ BIOS для всех тестов.
- `fw/Mashinist_DXE_driver_SerialIo_SerialIo.ffs` — GUID `97C81E5D-8FA0-486A-AAEA-0EFDF090FE4F`, 7675 байт.
- `fw/MAshinist_DXE_driver_TerminalSrc_TerminalSrc.ffs` — GUID `54891A9E-763E-4377-8841-8D5C90D88CDE`, 13403 байт.

**Не модифицируйте файлы в `fw/`**. Тестовые образы пишите в `/tmp/opencode/`.

### Ключевые GUID-ы в тестовом образе

| GUID | Тип | Описание |
|------|-----|----------|
| `8C8CE578-8A3D-4F1C-9935-896185C32DD3` | Volume | Первый том (FFSv2, 262072 байт) |
| `5C60F367-A505-419A-859E-2A4FF6CA6FE5` | Volume | Второй том (FFSv2, 216 файлов) — основной для тестов |
| `61C0F511-A691-4F54-974F-B9A42172CE53` | Volume | Третий том (FFSv2, 58 файлов) |
| `A0327FE0-1FDA-4E5B-905D-B510C45A61D0` | File | DXE-драйвер во втором томе (строка 214) — цель для insert-after |
| `CEF5B9A3-476D-497F-9FDC-E98143E0422C` | File | Первый файл в первом томе |
| `97C81E5D-8FA0-486A-AAEA-0EFDF090FE4F` | File | SerialIo — вставляемый файл |

### Регрессионные тесты (запускать после изменений в builder/ops)

```bash
cd /tmp/opencode

# 1. Rebuild без изменений = идентичность
UEFIEdit .../fw/HNX99TF_*.bin save /tmp/t1.bin && cmp /tmp/t1.bin .../fw/HNX99TF_*.bin && echo OK

# 2. Insert + extract = идентичность извлечённого
UEFIEdit .../fw/HNX99TF_*.bin insert-after A0327FE0-... .../fw/Mashinist_DXE_driver_SerialIo_SerialIo.ffs save /tmp/t2.bin
UEFIExtract /tmp/t2.bin all
cat "/tmp/t2.bin.dump/2 BIOS region/2 5C60F367-.../215 SerialIo/header.bin" \
    "/tmp/t2.bin.dump/2 BIOS region/2 5C60F367-.../215 SerialIo/body.bin" > /tmp/extracted.ffs
cmp /tmp/extracted.ffs .../fw/Mashinist_DXE_driver_SerialIo_SerialIo.ffs && echo OK

# 3. Insert + Remove = идентичность оригиналу
UEFIEdit .../fw/HNX99TF_*.bin insert-after A0327FE0-... SerialIo.ffs remove 97C81E5D-... save /tmp/t3.bin
cmp /tmp/t3.bin .../fw/HNX99TF_*.bin && echo OK

# 4. Remove Volume
UEFIEdit .../fw/HNX99TF_*.bin remove 5C60F367-... save /tmp/t4.bin && echo OK

# 5. Цепочка операций
UEFIEdit .../fw/HNX99TF_*.bin \
  insert 5C60F367-... SerialIo.ffs \
  insert-after A0327FE0-... TerminalSrc.ffs \
  remove 97C81E5D-... \
  save /tmp/t5.bin && echo OK
```

## Git

- Корневой репозиторий — форк [LongSoft/UEFITool](https://github.com/LongSoft/UEFITool) (ветка `new_engine`).
- `origin` — ваш форк на GitHub (push/pull), `upstream` — оригинальный LongSoft/UEFITool (только pull для синхронизации с обновлениями upstream).
- **Не делайте коммиты**, если явно не запрошено пользователем.
- При коммите следуйте стилю существующих сообщений: `"Improve ..."` / `"Add ..."` / `"Fix ..."` — короткое описание в настоящем времени, без префиксов типа `feat:`/`fix:`.

## Что не реализовано (известные ограничения)

- **Нет полной пересборки сжатых GUIDed-секций**: `buildSection` для compression-секций использует `compressData()` (Tiano/Efi), но LZMA/Brotli/Zlib-компрессия не реализована — только декомпрессия. Для LZMA-секций пересборка тела с повторным сжатием не сработает.
- **Нет переименования UI-секций**: поле `EFI_SECTION_USER_INTERFACE` (имя файла) не обновляется при insert.
- **Нет обновления FIT-таблицы**: при вставке/удалении микрокода FIT не пересчитывается.
- **Нет проверки свободного места перед вставкой**: `buildVolume` сообщает об ошибке только при сохранении, а не при insert.
- **Нет undo/redo** в GUI.
- **Builder messages dock** в GUI отключён (`enableDock(ui->builderMessagesDock, false)` в конструкторе) — можно включить для отладки.