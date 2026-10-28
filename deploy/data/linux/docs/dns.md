# DNS без systemd-resolved

## Как это устроено

`DnsUtilsLinux` настраивает DNS в два пути:

1. **systemd-resolved доступен** — используется D-Bus-интерфейс
   `org.freedesktop.resolve1` (как раньше):
   - `SetLinkDNS(ifindex, [(family, address)])`
   - `SetLinkDomains(ifindex, [(domain, search)])`
   - `SetLinkDefaultRoute(ifindex, bool)`
   - `RevertLink(ifindex)`
   - `Properties.Get("Domains")`

2. **systemd-resolved нет** — включается нативный fallback:
   - если найден `resolvconf` (openresolv), DNS применяется через
     `resolvconf -a amneziavpn -x -m 0`;
   - иначе демон пишет `/etc/resolv.conf` напрямую, сохраняя бэкап и тип
     исходного файла (обычный файл или симлинк) в `/var/lib/amneziavpn/`;
   - при отключении VPN или получении `SIGTERM` конфигурация
     восстанавливается.

Никакого Python, dbus-python или отдельного D-Bus-моста не нужно —
всё внутри демона.

## Зачем это нужно

Одновременно `LinuxFirewall` включает цепочку `amnvpn.a.310.blockDNS`:

```
-p udp --dport 53 -j REJECT
-p tcp --dport 53 -j REJECT
```

и разрешает DNS только в сторону серверов из конфига VPN через туннель.
Если резолвер не перенастроен, получается DNS-blackout: старый резолвер
заблокирован, новый не настроен. Отсюда «VPN подключился, а интернета нет».

## Зависимости

Обязательные:

- `iproute2` (`ip`);
- `iptables` или `nftables` — как и раньше, для firewall;
- сам клиент, собранный с нативным fallback (этот патч).

Опционально:

- `openresolv` / `resolvconf` — предпочтительный путь на OpenRC, runit,
  s6, dinit, SysVinit;
- `nscd` — если используется, демон чистит его кэш; на системах без
  systemd это не обязательно.

Если `resolvconf` нет, ничего устанавливать не нужно: демон аккуратно
правит `/etc/resolv.conf` сам.

## Проверка

```bash
# что реально в resolv.conf после подключения
cat /etc/resolv.conf

# резолвинг через туннель
dig +short example.com

# состояние сервиса
sudo rc-service amneziavpn status   # OpenRC
sudo sv status amneziavpn           # runit
```

Если `resolve1` на шине занят (systemd-resolved), демон идёт по D-Bus-пути.
На не-systemd системах этого имени, как правило, нет — сработает fallback.

## Утечки DNS

Проверить, что запросы идут через VPN:

```bash
# на сервере VPN
sudo tcpdump -ni any port 53

# на клиенте
dig +short whoami.akamai.net
```

Если в `resolv.conf` остался домашний роутер, а firewall его не блокирует —
это утечка. При штатной работе в `resolv.conf` должны быть только DNS
из конфига VPN.

## Аварийная диагностика

Если нужно срочно проверить связь, можно временно убрать блокировку DNS
(это утечка, только для диагностики):

```bash
sudo iptables -F amnvpn.a.310.blockDNS
```

После отключения VPN проверь, что цепочки не остались:

```bash
sudo iptables -S | grep amnvpn
```
