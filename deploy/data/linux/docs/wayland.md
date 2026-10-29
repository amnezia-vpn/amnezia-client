# Wayland и Qt-плагин

## Симптом

```
qt.qpa.plugin: Could not find the Qt platform plugin "wayland" in ""
```

Иногда после этого окно всё равно открывается (Qt падает обратно на `xcb`),
иногда приложение не стартует.

## Почему установка `qt6-wayland` не помогает

Официальный `AmneziaVPN_*_linux_x64.run` тащит **свой** Qt (в 5.0.x — 6.10.1)
в `plugins/` и `lib/`. Плагин платформы ищется в первую очередь рядом с
приложением. Системный `qt6-wayland` собран против другой версии Qt, и
загрузить его в бандл нельзя — не совпадёт ABI.

Поэтому есть три рабочих варианта.

## 1. XWayland (проще всего)

Обёртка `/usr/local/bin/AmneziaVPN`, которую ставит `install.sh`, делает это
автоматически: если сессия Wayland, а wayland-плагина в бандле нет —
выставляется `QT_QPA_PLATFORM=xcb`.

Вручную:

```bash
QT_QPA_PLATFORM=xcb /opt/AmneziaVPN/bin/AmneziaVPN
```

Или навсегда:

```bash
sudo ./install.sh --force-x11
```

Минус: нет нативного Wayland (масштабирование, дробный DPI). Для VPN-клиента
это обычно не критично.

## 2. Подложить плагин ровно той же версии Qt

Нужен `libqwayland*.so` от Qt **6.10.1** и его зависимости. На Arch это делает
AUR-пакет `amneziavpn-bin`:

```bash
bsdtar -xf qt6-wayland-6.10.1-1-x86_64.pkg.tar.zst -C /opt/AmneziaVPN \
  --strip-components=3 usr/lib/qt6/plugins/platforms/libqwayland.so
patchelf --set-rpath '$ORIGIN/../../lib' /opt/AmneziaVPN/plugins/platforms/libqwayland.so
```

На Gentoo можно собрать `qtwayland` той же версии, но проще вариант 1.

## 3. Собрать клиент из исходников

В upstream есть PR
[#3139](https://github.com/amnezia-vpn/amnezia-client/pull/3139) — «bundle
qtwayland in linux builds». Если он смержен в твою версию, плагин будет на месте.

## Как проверить, что плагин есть

```bash
ls /opt/AmneziaVPN/plugins/platforms/ | grep -i wayland
```

Если пусто — используется XWayland.

## Проверить, какая платформа реально загрузилась

```bash
QT_DEBUG_PLUGINS=1 /usr/local/bin/AmneziaVPN 2>&1 | grep -i "platform"
```
