# Slackware

Slackware 15 использует SysVinit-подобные скрипты в `/etc/rc.d/` и `rc.local`.
Установщик определит Slackware по `/etc/slackware-version` и положит сервис в
`/etc/rc.d/rc.amneziavpn`, добавив запуск в `/etc/rc.d/rc.local`.

## Зависимости

Отдельных Python-зависимостей нет: DNS настраивает сам демон (нативно, через
`resolvconf` или `/etc/resolv.conf`). Нужны только `iproute2` и
`iptables`/`nftables`, которые в Slackware и так есть.

Опционально можно поставить `openresolv` из SBo — тогда DNS будет
управляться через `resolvconf`, как на других init-системах.

## Установка

```bash
# клиент уже установлен официальным .run
sudo ./install.sh
```

## Управление

```bash
sudo /etc/rc.d/rc.amneziavpn start
sudo /etc/rc.d/rc.amneziavpn stop
sudo /etc/rc.d/rc.amneziavpn restart
sudo /etc/rc.d/rc.amneziavpn status
```

## Автозапуск

Установщик добавляет в `/etc/rc.d/rc.local`:

```sh
[ -x /etc/rc.d/rc.amneziavpn ] && /etc/rc.d/rc.amneziavpn start
```

Проверь, что `rc.local` исполняемый:

```bash
sudo chmod +x /etc/rc.d/rc.local
```
