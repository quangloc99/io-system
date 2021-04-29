# Лабораторная работа 2

**Название:** "Разработка драйверов блочных устройств"

**Цель работы:** Получить знания и навыки разработки драйверов блочных устройств для операционной 
системы Linux.

## Описание функциональности драйвера

Драйвер создает:
- Первичный раздел размером 10 Мбайт
- Расширенный раздел, содержащий два логических раздела по 20 Мбайт

...

## Инструкция по сборке
```
$ make                                  # создать модуль
$ sudo insmod io_lab2                   # установить модуль
$ sudo rmmod io_lab2                    # выгрузить модуль
```

...

## Инструкция пользователя
```
$ fdisk -l /dev/mydisk                      # посмотреть разделы виртуального диска.
$ mkfs.vfat /dev/mydisk1                    # форматировать раздел диска

$ mkdir -p /mnt/my-interesting-mount-point
$ mount /dev/mydisk1 /mnt/my-interesting-mount-point
```

...

## Примеры использования
```
$ mkfs.vfat /dev/mydisk1
$ mkdir -p /mnt/mydisk1
$ mount /dev/mydisk1 /mnt/mydisk1
$ echo "Never gonna give you up!" > /mnt/mydisk1/never-gonna-give-you-up
$ cat never-gonna-give-you-up
Never gonna give you up!
```

...

## Измерение скорости передачи файлов между реальными и виртуальными дисками
Измерение выпольняется с помощью команды `pv`.

Команды и результат измерения:
```
~$ cd /mnt
/mnt$ mkdir mydisk1 mydisk5
/mnt$ mkfs.vfat /dev/mydisk1
mkfs.fat 4.1 (2017-01-24)
/mnt$ mkfs.vfat /dev/mydisk5
mkfs.fat 4.1 (2017-01-24)
/mnt$ mount /dev/mydisk1 mydisk1
/mnt$ mount /dev/mydisk5 mydisk5
/mnt$ python -c "print('a' * (9 << 10 << 10))" > my-big-file
/mnt$ pv -pr my-big-file mydisk1
/mnt$ pv -pr my-big-file > mydisk1/my-big-file
[ 300MiB/s] [=================================================================================>] 100%
/mnt$ pv -pr mydisk1/my-big-file > mydisk5/my-big-file
[ 410MiB/s] [=================================================================================>] 100%
/mnt$ pv -pr mydisk5/my-big-file > my-big-file2
[ 524MiB/s] [=================================================================================>] 100%
/mnt$ pv -pr my-big-file2 > mydisk5/my-big-file2
[ 414MiB/s] [=================================================================================>] 100%
/mnt$ pv -pr mydisk5/my-big-file2 > mydisk1/my-big-file2
pv: write failed: No space left on device
/mnt$ rm mydisk1/my-big-file
/mnt$ pv -pr mydisk5/my-big-file2 > mydisk1/my-big-file2
[ 327MiB/s] [=================================================================================>] 100%
/mnt$ pv -pr mydisk1/my-big-file2 > my-big-file3
[ 577MiB/s] [=================================================================================>] 100%
```

Здесь я тестирую файл размером 9 МБ и пытался скопировать файл между всеми парами виртуальных/реальных 
дисков во всех направлениях. Результат каким-то образом предполагает, что место назначения ограничивает 
скорость (для mydisk1: около 300 МБ / с, для mydisk5: около 400 МБ / с и для реального диска: около 500 МБ / с)
