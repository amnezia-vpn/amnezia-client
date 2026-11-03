[English](README.md) | **Русский**

---

# AmneziaVPN на Linux без systemd

Официальный клиент **AmneziaVPN** на Gentoo/OpenRC, Void/runit, Artix, Alpine,
Devuan, Slackware, s6, dinit и любых других init-системах.

Привилегированный демон — обычный Qt-процесс, systemd ему не нужен. Это
изменение добавляет недостающие нативные части:

- настройку DNS без `systemd-resolved`;
- нативные сервисы для всех поддерживаемых init-систем;
- корректное завершение по `SIGTERM` и очистку firewall при остановке;
- без Python, `dbus-python` и внешнего DNS-моста.

---

## Зачем это нужно

systemd используется только в нескольких местах:

| Что | Где | Решение |
|---|---|---|
| Запуск и перезапуск демона | `post_install.sh`, `systemctl` | нативные init-скрипты + автодетект init |
| Настройка DNS | `DnsUtilsLinux` — D-Bus `org.freedesktop.resolve1` | нативный fallback на `resolvconf`/`/etc/resolv.conf` |
| Сброс DNS-кэша | `RouterLinux::flushDns()` — `systemctl restart nscd/systemd-resolved` | `nscd -i hosts`, иначе no-op |
| Привилегированные `ip` | `RouterLinux` — `sudo ip ...` | запускать напрямую, если уже root |
| Чистое завершение | `SIGTERM` не обрабатывался | подключить `SignalHandler` |

Главная проблема — DNS: при подключении демон включает iptables-цепочку
`amnvpn.a.310.blockDNS`, которая режет 53-й порт вне туннеля. Если резолвер
не перенастроен, ломается весь DNS — «VPN подключился, а интернета нет».

## Нативный fallback DNS

`DnsUtilsLinux` теперь работает в двух режимах:

1. **`org.freedesktop.resolve1` доступен** (systemd-resolved) — используется
   существующий D-Bus-путь без изменений.
2. **недоступен** — демон:
   - использует `resolvconf` (openresolv), если он установлен:
     `resolvconf -a amneziavpn -x -m 0`;
   - иначе пишет `/etc/resolv.conf` напрямую, сохраняя предыдущий файл и его
     тип (обычный файл или симлинк) в `/var/lib/amneziavpn/`;
   - восстанавливает исходную конфигурацию при отключении или `SIGTERM`.

Никакого Python, подмены D-Bus-имени и отдельного демона.

## Что делает установщик

1. Находит `AmneziaVPN-service` в `/opt/AmneziaVPN` (или в `--app-dir`).
2. Определяет init-систему и ставит нативный сервис с автоперезапуском:
   - **OpenRC** — `/etc/init.d/amneziavpn` (`supervise-daemon`)
   - **runit** — `/etc/sv/amneziavpn` (`runsv` + `svlogd`)
   - **s6** — `/etc/s6/sv/amneziavpn`
   - **dinit** — `/etc/dinit.d/amneziavpn`
   - **SysVinit / Slackware** — `/etc/init.d/amneziavpn` с watchdog
   - **systemd** — юнит с `Restart=always` (через существующий
     `post_install.sh`)
3. Чистит прыжки и цепочки `amnvpn.*` в `iptables`/`ip6tables` при остановке.
4. Ставит launcher `/usr/local/bin/AmneziaVPN`, который переключается на
   XWayland, если в bundled Qt нет Wayland-плагина.
5. Убирает старый `/etc/systemd/system/AmneziaVPN.service`, если он мешает.

## Автоматическая установка (официальный `.run`)

`post_install.sh` определяет, запущен ли systemd. Если да — работает старый
путь с юнитом; если нет — вызывает `install.sh --app-dir /opt/AmneziaVPN`
из состава пакета. `post_uninstall.sh` аналогично вызывает `uninstall.sh`.
В пакет попадают `install.sh`, `init/`, `lib/` и `docs/` (см.
`cmake/CPack.cmake`).

Скриптам нужен `bash`; на Alpine установи его заранее: `apk add bash`.

## Ручная установка

Для уже установленного клиента или самостоятельно собранного дерева:

```bash
sudo ./install.sh
```

`install.sh` предназначен для альтернативных init-систем. На systemd
существующий юнит ставит `post_install.sh` из состава пакета.

Опции:

```bash
sudo ./install.sh --force-x11   # всегда через XWayland
sudo ./install.sh --no-start    # установить, но не запускать
sudo ./install.sh --check       # показать, что будет сделано
```

Удаление:

```bash
sudo ./uninstall.sh          # снять сервисы, оставить /opt/AmneziaVPN
sudo ./uninstall.sh --purge  # удалить всё
```

## Требования

- `iproute2` (`ip`) и `iptables` или `nftables`;
- `bash`;
- клиент, собранный с этим нативным патчем — старый бинарник 5.0.x всё ещё
  умеет DNS только через D-Bus;
- опционально: `openresolv`/`resolvconf` для предпочтительного backend.

## Проверка

```bash
# OpenRC
sudo rc-service amneziavpn status
sudo tail -f /var/log/AmneziaVPN/amneziavpn.log

# runit
sudo sv status amneziavpn
sudo tail -f /var/log/AmneziaVPN/runit-amneziavpn/current

# после подключения
cat /etc/resolv.conf
dig +short example.com

# после отключения
cat /etc/resolv.conf

# не осталось цепочек firewall
sudo iptables -S | grep amnvpn
```

## Структура

```
install.sh / uninstall.sh
lib/common.sh                 детект init, поиск бинарников, шаблоны
init/<init>/service.template  шаблоны нативных сервисов
docs/                         wayland, dns, s6, slackware
```

Всё это попадает в официальный `.run` благодаря `cmake/CPack.cmake`.

## Тестирование

Жизненный цикл сервиса проверен в реальных контейнерах с официальным бинарником:

- **Void Linux / runit** — установка, создание сокета, респавн после
  `kill -9`, удаление, отсутствие зависших процессов;
- **Artix Linux / OpenRC** — то же + автозапуск через `rc-update`, имитация
  перезагрузки и очистка `iptables`/`ip6tables` во всех таблицах
  (`filter`, `nat`, `mangle`, `raw`);
- **systemd sandbox** — существующий путь с юнитом не изменился.

Нативный DNS-fallback проверен компиляцией с Qt 6.12 (`Qt6Core`, `Qt6DBus`,
`Qt6Network`). Полный тест через туннель требует сборки с этим патчем;
поведение сервиса и firewall от него не зависит.

Реальная проверка после перезагрузки возможна только на установленной
системе. В live ISO проверяй артефакт автозапуска — для OpenRC:
`ls -l /etc/runlevels/default/amneziavpn` и
`rc-update show default | grep amneziavpn`.

## Лицензия

GPL-3.0-or-later, как и сам AmneziaVPN.
