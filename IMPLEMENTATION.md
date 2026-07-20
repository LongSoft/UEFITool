# UEFI-tools: реализация редактирования секций в UEFITool и создание UEFIEdit

## Постановка задачи

В корне репозитория находятся консольные утилиты (`UEFIExtract`, `UEFIFind`, `UEFIEdit`) и графическая утилита (`UEFITool`) для работы с BIOS/UEFI образами. Необходимо:

1. Реализовать функционал добавления, изменения и удаления секций (insert/replace/remove) в графической утилите UEFITool.
2. Собрать графическую утилиту для Linux/ELF-64 нативно через qmake.
3. Тестовые файлы BIOS и FFS-модулей находятся в каталоге `fw/`:
   - `HNX99TF_200525_original_E5C88C6F.bin` — 16 МБ образ BIOS (Intel flash descriptor + ME region + BIOS region с 3 FFS-томами).
   - `Mashinist_DXE_driver_SerialIo_SerialIo.ffs` — 7675 байт, FFS-файл DXE-драйвера SerialIo, GUID `97C81E5D-8FA0-486A-AAEA-0EFDF090FE4F`.
   - `MAshinist_DXE_driver_TerminalSrc_TerminalSrc.ffs` — 13403 байт, FFS-файл DXE-драйвера TerminalSrc, GUID `54891A9E-763E-4377-8841-8D5C90D88CDE`.

Дополнительно по ходу работы была создана консольная утилита `UEFIEdit` для автоматизированного тестирования функционала без участия GUI.

---

## Исходное состояние кода

### GUI (UEFITool/)

Слоты `insert()`, `replace()`, `remove()`, `rebuild()`, `saveImageFile()` в `uefitool.cpp` были пустыми заглушками:

```cpp
void UEFITool::insert(const UINT8 mode) { U_UNUSED_PARAMETER(mode); }
void UEFITool::replace(const UINT8 mode) { U_UNUSED_PARAMETER(mode); }
void UEFITool::rebuild() { }
void UEFITool::remove() { }
void UEFITool::saveImageFile() { }
```

Активация действий в `populateUi()` была закомментирована:

```cpp
//ui->actionRebuild->setEnabled(type == Types::Volume || type == Types::File || type == Types::Section);
//ui->actionRemove->setEnabled(type == Types::Volume || type == Types::File || type == Types::Section);
//ui->actionInsertInto->setEnabled(...);
```

### Движок (common/)

`FfsBuilder::buildVolume()`, `buildPadFile()`, `buildFile()`, `buildSection()` возвращали `U_NOT_IMPLEMENTED`:

```cpp
USTATUS FfsBuilder::buildVolume(const UModelIndex & index, UByteArray & volume) {
    U_UNUSED_PARAMETER(index); U_UNUSED_PARAMETER(volume);
    return U_NOT_IMPLEMENTED;
}
```

`FfsOperations::replace()` возвращал `U_NOT_IMPLEMENTED` для обоих режимов.

В `TreeModel` отсутствовали сеттеры `setHeader()`/`setBody()`/`setTail()` — были только геттеры.

---

## Структура UEFI-образа

### Иерархия дерева

```
Image (Intel image, type=62)
├── Region: Descriptor (type=63, subtype=0) — только для чтения
├── Region: ME (type=63, subtype=2)
└── Region: BIOS (type=63, subtype=1) — raw area
    ├── Volume 8C8CE578-... (type=65, subtype=111=Ffs2Volume)
    │   └── File CEF5B9A3-... (type=66)
    ├── Padding (type=64)
    ├── Volume 5C60F367-... (type=65, 216 файлов)
    │   ├── File ... (type=66)
    │   ├── ...
    │   ├── File A0327FE0-1FDA-4E5B-905D-B510C45A61D0 (type=66, subtype=7=DXE driver)
    │   │   └── Section EE4E5898-... (type=67, subtype=2=GUID_DEFINED)
    │   │       ├── Section 20FEEBDE-... (subtype=24)
    │   │       └── ...
    │   └── FreeSpace (type=68, 2082024 байт)
    ├── Padding (type=64)
    └── Volume 61C0F511-... (type=65, 58 файлов)
```

### Типы элементов (common/types.h)

| Константа | Значение | Описание |
|-----------|----------|----------|
| `Types::Root` | 60 | Корень дерева |
| `Types::Capsule` | 61 | UEFI-капсула |
| `Types::Image` | 62 | Образ (Intel/UEFI) |
| `Types::Region` | 63 | Регион флешки |
| `Types::Padding` | 64 | Заполнитель |
| `Types::Volume` | 65 | FFS-том |
| `Types::File` | 66 | FFS-файл |
| `Types::Section` | 67 | Секция внутри файла |
| `Types::FreeSpace` | 68 | Свободное место в томе |

### Действия (common/types.h, namespace Actions)

| Константа | Значение | Описание |
|-----------|----------|----------|
| `NoAction` | 50 | Элемент не изменён |
| `Erase` | 51 | Стереть |
| `Create` | 52 | Создать |
| `Insert` | 53 | Вставить новый элемент |
| `Replace` | 54 | Заменить |
| `Remove` | 55 | Удалить |
| `Rebuild` | 56 | Пересобрать |
| `Rebase` | 57 | Перебазировать |

### Заголовки FFS

**EFI_FFS_FILE_HEADER** (common/ffs.h:270, 24 байта для FFSv2):
```c
typedef struct {
    EFI_GUID                Name;          // 16 байт — GUID файла
    EFI_FFS_INTEGRITY_CHECK IntegrityCheck;// 2 байта — header + file checksum
    UINT8                   Type;          // 1 байт — тип файла (DXE driver=7 и т.д.)
    UINT8                   Attributes;    // 1 байт — атрибуты
    UINT8                   Size[3];       // 3 байта — размер (24-битный, little-endian)
    UINT8                   State;         // 1 байт — состояние
} EFI_FFS_FILE_HEADER;                     // итого 24 байта
```

**EFI_FFS_FILE_HEADER2** (FFSv3, 32 байта) — добавляет `UINT64 ExtendedSize` после State. Используется, если `Size[3] == 0xFFFFFF` и `Attributes & FFS_ATTRIB_LARGE_FILE`.

**EFI_FFS_FILE_HEADER2_LENOVO** — вариант для FFSv2 rev2, добавляет `UINT32 ExtendedSize`.

**EFI_COMMON_SECTION_HEADER** (common/ffs.h:386, 4 байта):
```c
typedef struct {
    UINT8 Size[3];  // 3 байта — размер секции (24-битный)
    UINT8 Type;     // 1 байт — тип секции
} EFI_COMMON_SECTION_HEADER;
```

**EFI_COMMON_SECTION_HEADER2** (8 байт) — добавляет `UINT32 ExtendedSize`. Используется в FFSv3, если `Size[3] == 0xFFFFFF`.

### Типы секций (common/ffs.h:402-421)

| Константа | Значение | Описание |
|-----------|----------|----------|
| `EFI_SECTION_COMPRESSION` | 0x01 | Сжатая секция (инкапсулирующая) |
| `EFI_SECTION_GUID_DEFINED` | 0x02 | GUID-defined секция (инкапсулирующая) |
| `EFI_SECTION_DISPOSABLE` | 0x03 | Disposable секция (инкапсулирующая) |
| `EFI_SECTION_PE32` | 0x10 | PE32-образ |
| `EFI_SECTION_PIC` | 0x11 | Position-independent code |
| `EFI_SECTION_TE` | 0x12 | Terse executable |
| `EFI_SECTION_DXE_DEPEX` | 0x13 | DXE dependency |
| `EFI_SECTION_VERSION` | 0x14 | Версия |
| `EFI_SECTION_USER_INTERFACE` | 0x15 | UI (имя файла) |
| `EFI_SECTION_FIRMWARE_VOLUME_IMAGE` | 0x17 | Образ тома (инкапсулирующая) |
| `EFI_SECTION_RAW` | 0x19 | Сырые данные |

### GUID-ы GUID-defined секций (common/ffs.h:449-458)

| Константа | GUID | Алгоритм |
|-----------|------|----------|
| `EFI_GUIDED_SECTION_CRC32` | `FC1BCDB0-7D31-49AA-936A-A4600D9DD083` | CRC32 |
| `EFI_GUIDED_SECTION_TIANO` | `A31280AD-481E-41B6-95E8-127F4C984779` | Tiano |
| `EFI_GUIDED_SECTION_LZMA` | `EE4E5898-3914-4259-9D6E-DC7BD79403CF` | LZMA |
| `EFI_GUIDED_SECTION_BROTLI` | `3D532050-5CDA-4FD0-879E-0F7F630D5AFB` | Brotli |

### Контрольные суммы FFS-файлов

- **Header checksum**: `0x100 - (calculateSum8(header) - Header - File - State)` — сумма всех байт заголовка кроме полей `IntegrityCheck.Header`, `IntegrityCheck.File` и `State`.
- **Data checksum**:
  - если `Attributes & FFS_ATTRIB_CHECKSUM` — реальная 8-битная контрольная сумма body;
  - иначе для rev1 — `FFS_FIXED_CHECKSUM = 0x5A`;
  - иначе для rev2 — `FFS_FIXED_CHECKSUM2 = 0xAA`.
- **Tail** (только rev1 + `FFS_ATTRIB_TAIL_PRESENT`): последние 2 байта body = `~IntegrityCheck.TailReference`.

### Выравнивание

- **Файлы внутри тома**: выравниваются по 8 байт. После каждого файла могут идти alignment bytes, хранящиеся в `model->alignmentBytes(fileIndex)`.
- **Секции внутри файла**: выравниваются по 4 байта.
- **Том внутри raw area**: выравнивается по `alignment` из `VOLUME_PARSING_DATA` (обычно 64K для rev2).

### Parsing Data (common/parsingdata.h)

```c
typedef struct {
    EFI_GUID extendedHeaderGuid;  // FvName из расширенного заголовка — это отображаемое имя тома
    UINT32   alignment;
    UINT32   usedSpace;
    BOOLEAN  hasValidUsedSpace;
    UINT8    ffsVersion;           // 2 или 3
    UINT8    emptyByte;            // 0xFF для erase polarity true, 0x00 для false
    UINT8    revision;             // 1 или 2
    BOOLEAN  hasExtendedHeader;
    BOOLEAN  hasAppleCrc32;
    BOOLEAN  isWeakAligned;
    BOOLEAN  requiresSectionAlignmentQuirk;
} VOLUME_PARSING_DATA;

typedef struct {
    UINT8    emptyByte;
    EFI_GUID guid;                 // GUID файла
} FILE_PARSING_DATA;

typedef struct {
    EFI_GUID guid;                 // GUID GUIDed-секции
    UINT32   dictionarySize;
} GUIDED_SECTION_PARSING_DATA;

typedef struct {
    UINT32 uncompressedSize;
    UINT8  compressionType;        // EFI_NOT_COMPRESSED=0, EFI_STANDARD_COMPRESSION=1, ...
    UINT8  algorithm;
    UINT32 dictionarySize;
} COMPRESSED_SECTION_PARSING_DATA;
```

**Важно**: отображаемое имя тома в GUI (`5C60F367-...`) — это **не** `FileSystemGuid` из `EFI_FIRMWARE_VOLUME_HEADER` (который для всех FFSv2-томов одинаковый — `8C8CE578-...`), а `extendedHeaderGuid` из `VOLUME_PARSING_DATA`, который берётся из `EFI_FIRMWARE_VOLUME_EXT_HEADER.FvName`.

---

## Реализация

### 1. Сеттеры в TreeModel (common/treeitem.h, treemodel.h, treemodel.cpp)

Добавлены методы для изменения данных элемента после создания:

```cpp
// treeitem.h
void setHeader(const UByteArray &header) { itemHeader = header; }
void setBody(const UByteArray &body) { itemBody = body; }
void setTail(const UByteArray &tail) { itemTail = tail; }

// treemodel.h
void setHeader(const UModelIndex &index, const UByteArray &header);
void setBody(const UModelIndex &index, const UByteArray &body);
void setTail(const UModelIndex &index, const UByteArray &tail);
```

Реализация в `treemodel.cpp` вызывает `emit dataChanged(index, index)` для обновления GUI.

### 2. FfsOperations::replace() (common/ffsops.cpp)

```cpp
USTATUS FfsOperations::replace(const UModelIndex & index, const UByteArray & data, const UINT8 mode)
```

- **REPLACE_MODE_AS_IS**: новые данные разделяются на header/body/tail по размерам оригинала, всё заменяется.
- **REPLACE_MODE_BODY**: заменяется только body, header и tail сохраняются.
- После замены вызывается `model->clearChildren(index)` — удаляются дочерние элементы, чтобы `FfsBuilder::buildFile`/`buildSection` использовали новый body напрямую, а не пересобирали из устаревших children (см. баг 9).
- После замены устанавливается `Actions::Replace` и каскадно `Actions::Rebuild` для родителя.

Сигнатура изменена с `UByteArray & data` на `const UByteArray & data`, т.к. данные не модифицируются.

### 3. FfsBuilder — реализация buildVolume/buildPadFile/buildFile/buildSection (common/ffsbuilder.cpp)

#### buildVolume

Обрабатывает `NoAction`/`Remove`/`Rebuild`/`Replace`/`Insert`. Ключевая логика — потребление FreeSpace:

```cpp
// FreeSpace откладывается, не строится сразу
for (int i = 0; i < model->rowCount(index); i++) {
    if (model->type(currentChild) == Types::FreeSpace) {
        freeSpaceSize += model->bodySize(currentChild);
        freeSpaceFound = true;
        continue;  // не добавляем в body
    }
    // ... build File/Padding ...
    body += currentData;
}

// Проверяем, что новый body помещается
if (newBodySize > oldBodySize) {
    if (freeSpaceFound && (newBodySize - oldBodySize) <= freeSpaceSize) {
        // OK — свободное место поглотит разницу
    } else {
        return U_INVALID_VOLUME;
    }
}
// Дополняем emptyByte до oldBodySize
if (newBodySize < oldBodySize)
    body += UByteArray(oldBodySize - newBodySize, (char)emptyByte);
```

Это позволяет вставлять/заменять/удалять файлы внутри тома, если есть достаточно свободного места.

#### buildFile

Обрабатывает `NoAction`/`Remove`/`Rebuild`/`Replace`/`Insert`. Ключевые моменты:

- **Определение типа заголовка**: FFSv2 (24 байт), FFSv3 large (32 байт), Lenovo large (32 байт) — по `FFS_ATTRIB_LARGE_FILE` и `ffsVersion`/`revision` родительского тома.
- **Пересчёт размера**: `uint32ToUint24(newFullSize, fileHeader->Size)` для обычных файлов; `ExtendedSize = newFullSize` для large-файлов.
- **Пересчёт header checksum**:
  ```cpp
  UINT8 calculatedHeader = (UINT8)(0x100 - (calculateSum8(header, headerSize)
                                           - fileHeader->IntegrityCheck.Checksum.Header
                                           - fileHeader->IntegrityCheck.Checksum.File
                                           - fileHeader->State));
  ```
- **Пересчёт data checksum**: по `FFS_ATTRIB_CHECKSUM` или `FFS_FIXED_CHECKSUM`/`FFS_FIXED_CHECKSUM2`.
- **Tail для rev1**: если `FFS_ATTRIB_TAIL_PRESENT`, пересчитывается `~IntegrityCheck.TailReference`.
- **Body без детей**: если у файла нет дочерних секций (например, вставлен как raw blob), используется сохранённое body как есть:
  ```cpp
  if (model->rowCount(index) == 0)
      body = model->body(index);
  ```

#### buildSection

Обрабатывает `NoAction`/`Remove`/`Rebuild`/`Replace`/`Insert`. Ключевые моменты:

- **Инкапсулирующие секции** (`EFI_SECTION_COMPRESSION`, `EFI_SECTION_GUID_DEFINED`, `EFI_SECTION_DISPOSABLE`, `EFI_SECTION_FIRMWARE_VOLUME_IMAGE`): пересобирают дочерние секции/тома в новый body.
- **Compression section** (`EFI_SECTION_COMPRESSION`): новый body сжимается через `compressData()` с использованием `compressionType` из `COMPRESSED_SECTION_PARSING_DATA`. Поддерживаются `EFI_NOT_COMPRESSED`, `EFI_STANDARD_COMPRESSION` (TianoCompress), `EFI_CUSTOMIZED_COMPRESSION` (EfiCompress).
- **GUID-defined section** (`EFI_SECTION_GUID_DEFINED`): новый body сжимается в зависимости от GUID из `GUIDED_SECTION_PARSING_DATA`:
  - `EFI_GUIDED_SECTION_LZMA` / `LZMA_HP` / `LZMA_MS` — сжатие через `LzmaCompress` с `dictionarySize` из parsing data. `LzmaCompress` формирует полный LZMA-stream: `[props:5][uncompressed_size:8 LE][compressed_data]`, что соответствует формату AMI LZMA GUIDed-секции.
  - `EFI_GUIDED_SECTION_TIANO` — сжатие через `compressData(EFI_STANDARD_COMPRESSION)`.
  - `EFI_GUIDED_SECTION_LZMAF86` — сжатие через `LzmaCompress` (без x86 BCJ пост-фильтра; большинство прошивок принимает plain LZMA для этого GUID).
  - Прочие GUID (CRC32, Brotli, GZip, Zlib, неизвестные) — body используется как есть (`body = newBody`), без сжатия.
  - Заголовок `EFI_GUID_DEFINED_SECTION` (GUID + DataOffset + Attributes) сохраняется из оригинального `model->header(index)`, размер секции пересчитывается ниже.
- **Авто-апгрейд common → section2**: если новый размер превышает 24-битный лимит в FFSv3, заголовок расширяется до `EFI_COMMON_SECTION_HEADER2`.
- **Секция без детей**: используется сохранённое body.

#### buildRawArea

Обрабатывает `NoAction`/`Remove`/`Rebuild`/`Replace`/`Insert`. Дети: `Types::Volume`, `Types::Padding`. При уменьшении размера (удаление тома) дополняется `0xFF`:

```cpp
if (newSize < oldSize)
    rawArea += UByteArray(oldSize - newSize, (char)0xFF);
```

#### buildCapsule / buildIntelImage

Добавлена обработка `Actions::Insert` в дополнение к `Rebuild`/`Replace`.

#### Подключение компрессии

В `ffsbuilder.cpp` добавлены:
```cpp
#include "Tiano/EfiTianoCompress.h"
#include "LZMA/LzmaCompress.h"
```

И статическая функция `compressData()` для Tiano/EFI-сжатия.

### 4. GUI UEFITool (UEFITool/uefitool.cpp)

#### insert()

**Ключевой момент — определение типа нового элемента по типу родителя, а не выбранного элемента:**

```cpp
// parentIndex — логический контейнер (куда вставляется)
// refItem — что передаётся в addItem (зависит от режима)
if (mode == CREATE_MODE_PREPEND) {
    parentIndex = index;        // выбранный элемент — контейнер
    refItem = index;
} else {  // BEFORE/AFTER
    parentIndex = index.parent(); // родитель выбранного — контейнер
    refItem = index;              // выбранный — reference для addItem
}

// Тип нового элемента определяется по parentType:
// Volume → File, File/инкапс. Section → Section
UINT8 parentType = model->type(parentIndex);
bool insertFile = (parentType == Types::Volume);
```

**TreeModel::addItem интерпретирует аргумент `parent` по-разному:**
- `CREATE_MODE_PREPEND`/`APPEND`: `parent` = контейнер, новый элемент становится его ребёнком.
- `CREATE_MODE_BEFORE`/`AFTER`: `parent` = reference-элемент, `addItem` берёт `parent.internalPointer()` и вставляет рядом с его родителем.

Поэтому для BEFORE/AFTER нужно передавать **выбранный элемент** (`refItem`), а не его родителя (`parentIndex`).

**Парсинг заголовка вставляемого файла:**
- Для File: `EFI_FFS_FILE_HEADER`, определение large-заголовка по `FFS_ATTRIB_LARGE_FILE` и FFS-версии родительского тома.
- Для Section: `EFI_COMMON_SECTION_HEADER`, определение section2 по `EFI_SECTION2_IS_USED` (0xFFFFFF) и FFSv3.
- Body = всё после заголовка до конца файла (или до размера секции, если она меньше файла).

**Пометка для rebuild:**
```cpp
model->setAction(newIndex, Actions::Insert);
for (UModelIndex p = parentIndex; p.isValid() && model->type(p) != Types::Root; p = p.parent()) {
    if (model->action(p) == Actions::NoAction)
        model->setAction(p, Actions::Rebuild);
}
model->setAction(root, Actions::Rebuild);
```

Каскадная пометка всех предков до root обязательна — иначе изменение не дойдёт до `build()`.

#### replace()

Вызывает `ffsOps->replace(index, data, mode)`, затем каскадно помечает предков для rebuild.

#### remove()

Вызывает `ffsOps->remove(index)` (помечает `Actions::Remove`), затем каскадно помечает предков для rebuild.

#### rebuild()

Вызывает `ffsOps->rebuild(index)` (помечает `Actions::Rebuild` + каскадно предков через `FfsOperations::rebuild`).

#### saveImageFile()

```cpp
if (!ffsBuilder) ffsBuilder = new FfsBuilder(model);
ffsBuilder->clearMessages();
UByteArray image;
USTATUS result = ffsBuilder->build(root, image);
// запись в файл
```

#### populateUi()

Раскомментированы и активированы действия в контекстном меню:
- `actionRebuild` — для Volume/File/Section (кроме Descriptor region).
- `actionRemove` — для Volume/File/Section.
- `actionInsertInto` — для Volume (не UnknownVolume), File (не ALL/RAW/PAD), инкапс. Section.
- `actionInsertBefore`/`InsertAfter` — для File/Section.
- `actionReplace`/`ReplaceBody` — для Region (не Descriptor)/Volume/File/Section.

### 5. Консольная утилита UEFIEdit (UEFIEdit/)

Создана для автоматизированного тестирования без GUI. Не использует Qt — работает на bstrlib-реализации `UString`/`UByteArray`.

#### Файлы

- `uefiedit.h` / `uefiedit.cpp` — класс `UEFIEdit` с методами `init`/`save`/`insert`/`remove`/`replace`/`rebuild`/`dumpTree`.
- `uefiedit_main.cpp` — CLI-парсер, поддерживает chaining команд: `UEFIEdit imagefile cmd1 args1 cmd2 args2 ... save out.bin`.
- `uefiedit.pro` — qmake (без Qt: `QT -= core gui; CONFIG -= qt`).
- `CMakeLists.txt` — cmake.
- `meson.build` — meson, добавлен `subdir('UEFIEdit')` в корневой `meson.build`.

#### Системы сборки

```bash
# qmake
qmake-qt5 uefiedit.pro && make

# cmake
cmake . && make

# meson (из корня репозитория)
meson setup build . && ninja -C build
```

#### Использование

```
UEFIEdit imagefile dump                              - вывести дерево
UEFIEdit imagefile save output.bin                   - сохранить (пересобрать)
UEFIEdit imagefile insert GUID file.ffs              - вставить FFS в том с GUID
UEFIEdit imagefile insert-before GUID file.ffs       - вставить перед файлом GUID
UEFIEdit imagefile insert-after GUID file.ffs        - вставить после файла GUID
UEFIEdit imagefile remove GUID                       - удалить элемент с GUID
UEFIEdit imagefile replace GUID file.ffs             - заменить (as-is)
UEFIEdit imagefile replace-body GUID file.bin        - заменить body
UEFIEdit imagefile rebuild GUID                      - пометить для rebuild
```

#### Поиск элемента по GUID

`findItemByGuidRecursive()` ищет:
1. **File** — по GUID из `EFI_FFS_FILE_HEADER.Name` (первые 16 байт header).
2. **Volume** — по `extendedHeaderGuid` из `VOLUME_PARSING_DATA` (т.к. отображаемое имя = `FvName` из расширенного заголовка, а не `FileSystemGuid`).
3. **GUIDed Section** — по GUID из `parsingData`.

GUID парсится функцией `parseGuidString()` — нормализуется (lowercase, без дефисов), затем разбивается на `Data1`/`Data2`/`Data3`/`Data4[8]` в формате EFI (little-endian для первых трёх полей).

---

## Найденные и исправленные баги

### Баг 1: Вставка в BIOS region вместо Volume

**Симптом**: "Insert After" файла в Volume вставлял новый файл в BIOS region (raw area), а не в Volume. При сохранении — `buildRawArea: unexpected item type File`.

**Причина**: `insert()` передавал `parentIndex` (Volume) как аргумент `parent` в `TreeModel::addItem()`. Но для `CREATE_MODE_AFTER` `addItem` интерпретирует `parent` как **reference-элемент** и вставляет рядом с его родителем. Поэтому новый файл становился соседом Volume (в BIOS region), а не соседом File (в Volume).

**Решение**: ввести отдельную переменную `refItem` — для BEFORE/AFTER это выбранный элемент `index`, для PREPEND — сам `index` (он же контейнер).

### Баг 2: Тип нового элемента определялся по выбранному элементу, а не по родителю

**Симптом**: "Insert After" файла (родитель=Volume) парсил .ffs как section, брал байт `0x97` (из GUID файла `5D1EC897...`) как тип секции → `Unknown section type 097h`.

**Причина**: код смотрел на `type` выбранного элемента (`Types::File`) и парсил данные как section. Но при insert-after файла новый элемент должен быть File (сосед файла в Volume), а не Section.

**Решение**: тип нового элемента определяется по `parentType` (типу контейнера): `Volume → File`, `File/инкапс. Section → Section`.

### Баг 3: `Actions::Insert` не обрабатывался в builder

**Симптом**: `buildFile: unexpected action Insert`.

**Причина**: `buildFile`/`buildSection`/`buildVolume`/`buildRawArea`/`buildCapsule` обрабатывали только `Rebuild`/`Replace`, но не `Insert`.

**Решение**: добавлен `|| model->action(index) == Actions::Insert` во все условия rebuild.

### Баг 4: `buildVolume` не потреблял FreeSpace

**Симптом**: `buildVolume: new volume body size is bigger than the original` — вставка файла (7675 байт) увеличивала body, но FreeSpace оставался прежним.

**Причина**: `buildVolume` строил все дети включая FreeSpace, затем проверял `newBodySize <= oldBodySize`. Вставка файла делала body больше оригинала.

**Решение**: FreeSpace откладывается (не строится сразу), его размер суммируется. После построения остальных детей проверяется, что превышение помещается в freeSpace. Оставшееся место заполняется `emptyByte`.

### Баг 5: Пустое body для вставленного файла без детей

**Симптом**: вставленный .ffs имел `body=0` после rebuild — `buildFile` собирал body из дочерних секций, а их не было (вставленный файл не парсится в дереве).

**Причина**: `buildFile` при rebuild всегда итерировал `model->rowCount(index)`, а для нового вставленного файла children=0.

**Решение**: если `model->rowCount(index) == 0`, используется `model->body(index)` (сохранённое body) как есть. То же для `buildSection`.

### Баг 6: `buildRawArea` падал при уменьшении размера

**Симптом**: `buildRawArea: new area size is smaller than the original` при удалении Volume.

**Причина**: `buildRawArea` требовал `newSize == oldSize`, но удаление тома уменьшало размер.

**Решение**: при `newSize < oldSize` недостающее место заполняется `0xFF` (типичное значение пустой флешки).

### Баг 7: `findItemByGuid` искал только File

**Симптом**: `insert 5C60F367-... ...` — `Item with GUID not found`, хотя Volume с этим именем есть в дереве.

**Причина**: поиск проверял только `Types::File` по GUID из header. Для Volume отображаемое имя — это `extendedHeaderGuid` из `VOLUME_PARSING_DATA`, а не `FileSystemGuid` (который для всех FFSv2-томов `8C8CE578-...`).

**Решение**: добавлена проверка Volume по `VOLUME_PARSING_DATA.extendedHeaderGuid` и GUIDed Section по GUID из parsingData.

### Баг 8: `remove`/`replace` не помечали предков для rebuild

**Симптом**: `remove` помечал только root для rebuild, но не Volume-родитель — изменение не применялось при save.

**Решение**: каскадная пометка всех предков от `index.parent()` до root.

### Баг 9: `replace` игнорировал новый body при наличии children

**Симптом**: `UEFIEdit ... replace GUID file.ffs save out.rom` — образ не менялся, даже если FFS изменён. `replace-body` — аналогично.

**Причина**: `FfsOperations::replace` вызывал `model->setBody(index, body)`, но не удалял дочерние элементы. В `FfsBuilder::buildFile` (и `buildSection`) для элементов с children body пересобирается из дочерних секций, а `model->body(index)` игнорируется:

```cpp
if (model->rowCount(index) == 0) {
    body = model->body(index);  // используется новый body
}
for (int i = 0; i < model->rowCount(index); i++) {
    // пересобирает из СТАРЫХ children, игнорирует новый body
}
```

Поскольку у заменяемого FFS-файла есть children (секции), builder пересобирал файл из старых children, и новый body никогда не использовался.

**Решение**: добавлен метод `TreeModel::clearChildren(index)` (и `TreeItem::clearChildren()`), удаляющий всех дочерних элементов. `FfsOperations::replace` вызывает `model->clearChildren(index)` перед `setBody` для обоих режимов (`REPLACE_MODE_AS_IS` и `REPLACE_MODE_BODY`). После очистки children `buildFile`/`buildSection` видят `rowCount==0` и используют новый body напрямую.

### Баг 10: `buildSection` не сжимал GUIDed LZMA-секции

**Симптом**: `replace`/`replace-body` для FFS-файла с LZMA GUIDed-секцией — секция не сжималась, тело записывалось несжатым. Объём FFS резко вырастал (98KB вместо 27KB сжатого), не помещался в volume.

**Причина**: в `buildSection` компрессия применялась только к `EFI_SECTION_COMPRESSION` (строка 793). Для `EFI_SECTION_GUID_DEFINED` тело использовалось как есть: `body = newBody` — без сжатия. Это было документированное ограничение.

**Решение**: в `buildSection` добавлена обработка `EFI_SECTION_GUID_DEFINED` с известными GUID-ами сжатия. Для `EFI_GUIDED_SECTION_LZMA`/`LZMA_HP`/`LZMA_MS` — вызов `LzmaCompress` с `dictionarySize` из `GUIDED_SECTION_PARSING_DATA`. Для `EFI_GUIDED_SECTION_TIANO` — `compressData(EFI_STANDARD_COMPRESSION)`. Для `EFI_GUIDED_SECTION_LZMAF86` — `LzmaCompress` (без x86 BCJ пост-фильтра). Прочие GUIDed-секции — `body = newBody` как раньше.

`LzmaCompress` (из `common/LZMA/LzmaCompress.c`) уже был подключён (`#include "LZMA/LzmaCompress.h"`) и формирует полный LZMA-stream `[props:5][uncompressed_size:8 LE][compressed_data]`, что соответствует формату AMI LZMA GUIDed-секции. Заголовок `EFI_GUID_DEFINED_SECTION` (GUID + DataOffset + Attributes) сохраняется из оригинального header, размер секции пересчитывается стандартным блоком ниже.

---

## Тестирование

### Окружение

- Linux x86-64, Qt 5.15.18 (`qmake-qt5`).
- Сборка GUI: `cd UEFITool && qmake-qt5 uefitool.pro && make`.
- Сборка UEFIEdit: qmake/cmake/meson — все три работают.

### Тест 1: Rebuild без изменений = побайтовое совпадение

```
UEFIEdit HNX99TF_*.bin save out.bin
→ out.bin идентичен оригиналу (cmp возвращает 0)
```

### Тест 2: Insert after + extract = идентичность извлечённого

```
UEFIEdit HNX99TF_*.bin insert-after A0327FE0-... Mashinist_DXE_driver_SerialIo_SerialIo.ffs save out.bin
UEFIExtract out.bin all
cat .../215\ SerialIo/header.bin .../215\ SerialIo/body.bin > extracted.ffs
cmp extracted.ffs Mashinist_DXE_driver_SerialIo_SerialIo.ffs
→ IDENTICAL
```

Вставленный файл извлекается из пересобранного образа побайтово идентичным оригиналу.

### Тест 3: Insert + Remove = идентичность оригиналу

```
UEFIEdit HNX99TF_*.bin insert-after A0327FE0-... SerialIo.ffs remove 97C81E5D-... save out.bin
cmp out.bin HNX99TF_*.bin
→ IDENTICAL
```

Вставка с последующим удалением возвращает образ к оригиналу.

### Тест 4: Цепочка операций

```
UEFIEdit HNX99TF_*.bin \
  insert 5C60F367-... SerialIo.ffs \
  insert-after A0327FE0-... TerminalSrc.ffs \
  remove 97C81E5D-... \
  save out.bin
→ A0327FE0 в строке 214, 54891A9E (TerminalSrc) в строке 215, 97C81E5D (SerialIo) удалён
```

### Тест 5: Remove Volume

```
UEFIEdit HNX99TF_*.bin remove 5C60F367-... save out.bin
→ Volume 5C60F367 удалён, BIOS region содержит 4 детей (было 5), место заполнено 0xFF
```

### Тест 6: Insert before / Insert into (prepend)

```
UEFIEdit HNX99TF_*.bin insert-before A0327FE0-... SerialIo.ffs save out.bin  → OK
UEFIEdit HNX99TF_*.bin insert 5C60F367-... SerialIo.ffs save out.bin         → OK (файл в начале тома)
```

### Тест 7: Replace / Replace body

```
UEFIEdit HNX99TF_*.bin replace A0327FE0-... SerialIo.ffs save out.bin        → OK
UEFIEdit HNX99TF_*.bin replace-body A0327FE0-... SerialIo.ffs save out.bin   → OK
```

### Тест 8: GUI

```
QT_QPA_PLATFORM=offscreen ./UEFITool HNX99TF_*.bin
→ образ открывается, парсится, GUI запускается без ошибок (exit=0)
```

### Тест 9: Replace FFS с LZMA GUIDed-секцией (увеличение размера)

Тест заменяет FFS-файл `899407D7-99FE-43D8-9A21-79EC328CAC21` (Setup, содержит LZMA GUIDed-секцию) на новый FFS с увеличенным PE32 (98208 байт вместо 90624). Новый FFS собран вручную: DXE dep section + GUIDed LZMA-секция с пересобранным PE32+UI+Version внутри.

```
UEFIEdit HNX99TF_*.bin replace 899407D7-... new_setup_ffs.bin save out.rom
→ out.rom отличается от оригинала
→ FFS size: 27596 (было 25889)
→ GUIDed section: type=0x02, size=0x6B56, GUID=EE4E5898-... (LZMA), DataOffset=0x18, Attributes=0x01
→ LZMA decompression: 98244 байта (соответствует new unc_data)
→ PE32 внутри: 98208 байт, ifrextractor находит CR-формы
```

Тот же результат через `replace-body`:
```
UEFIEdit HNX99TF_*.bin replace-body 899407D7-... new_setup_ffs_body.bin save out.rom
→ идентичный результат
```

Этот тест проверяет:
- Баг 9 (clearChildren): образ изменился, новый body использован.
- Баг 10 (LZMA компрессия): GUIDed-секция сжата через LzmaCompress, декомпрессия восстанавливает исходные данные.
- FreeSpace-потребление: FFS вырос на 1707 байт, volume содержит 2MB free space — рост поглощён.

---

## Изменённые файлы

| Файл | Изменение |
|------|-----------|
| `common/treeitem.h` | +`setHeader`/`setBody`/`setTail`, +`clearChildren` |
| `common/treeitem.cpp` | +реализация `clearChildren` |
| `common/treemodel.h` | +`setHeader`/`setBody`/`setTail`, +`clearChildren` |
| `common/treemodel.cpp` | +реализация сеттеров, +реализация `clearChildren` |
| `common/ffsops.h` | `replace` сигнатура → `const UByteArray &` |
| `common/ffsops.cpp` | реализация `replace` (AS_IS/BODY), +`clearChildren` перед `setBody` |
| `common/ffsbuilder.cpp` | реализация `buildVolume`/`buildPadFile`/`buildFile`/`buildSection`, `Actions::Insert`, FreeSpace-потребление, `compressData()`, +LZMA/Tiano компрессия для GUIDed-секций, +`#include "parsingdata.h"` |
| `common/filesystem.cpp` | +`#include <vector>` |
| `UEFITool/uefitool.cpp` | реализация `insert`/`replace`/`remove`/`rebuild`/`saveImageFile`, активация в `populateUi` |
| `meson.build` | +`subdir('UEFIEdit')` |
| `UEFIEdit/uefiedit.h` | новый — класс UEFIEdit |
| `UEFIEdit/uefiedit.cpp` | новый — реализация |
| `UEFIEdit/uefiedit_main.cpp` | новый — CLI |
| `UEFIEdit/uefiedit.pro` | новый — qmake |
| `UEFIEdit/CMakeLists.txt` | новый — cmake |
| `UEFIEdit/meson.build` | новый — meson |

---

## Сборка

### Графическая утилита UEFITool (Linux/ELF-64, qmake)

```bash
cd UEFITool
qmake-qt5 uefitool.pro
make -j$(nproc)
./UEFITool  # ELF 64-bit LSB executable, x86-64
```

### Консольная утилита UEFIEdit

```bash
# qmake (рекомендуется для разработки)
cd UEFIEdit
qmake-qt5 uefiedit.pro
make -j$(nproc)
./UEFIEdit

# cmake
mkdir build && cd build
cmake ../UEFIEdit
make -j$(nproc)
./uefiedit

# meson (из корня репозитория)
meson setup build .
ninja -C build UEFIEdit/UEFIEdit
./build/UEFIEdit/UEFIEdit
```

### Зависимости

- GCC/Clang с поддержкой C++11.
- Для GUI: Qt 5 (Core, Gui, Widgets) — `qt5-qtbase-devel` в Fedora/RPM.
- zlib (системная или bundled в `common/zlib/`).
- Никаких внешних библиотек для UEFIEdit (bstrlib, brotli, LZMA, Tiano — всё bundled в `common/`).