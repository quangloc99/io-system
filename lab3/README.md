# Лабораторная работа 3

**Название:** "Разработка драйверов сетевых устройств"

**Цель работы:**  Получить знания и навыки разработки драйверов сетевых устройств под операционную
систему Linux

## Описание функциональности драйвера

Драйвер создаёт виртуальное сетевое устройство, которое перехватывает пакеты протокола ICMP типа 8 
- Echo Request и выводит данные, передаванмые в пакете, в кольцевой буфер.

Драйвер также создаёт файл `/proc/if_var1`, из которого можно считать статистику прехваченных 
пакетов.

...

## Инструкция по сборке

    make

...

## Инструкция пользователя
#### Устоновка модуля:

    sudo insmod virt_net_if.ko [link=[parent interface name]]`

`link` по умольчанию - `enp0s3`

#### Отключение модуля:

    sudo insmod virt_net_if

#### Чтение сообщений из кольцевого буфера:

    dmesg

#### Чтение состояния сетевого устройства из `/proc/if_var1`:

    cat /proc/vni_stats

...

## Примеры использования

    ping www.google.com -c 3

Затем вызывать команду `dmesg`, чтобы увидеть адрес источники, адрес приёмники и данные пакета.
...

vim: wrap
