#!/usr/bin/env python3
"""
Скрипт для конвертации WireGuard конфигурации в vpn:// ссылку формата Amnezia Client.

Использование:
    python3 wireguard_to_vpn_link.py
    
или с параметрами:
    python3 wireguard_to_vpn_link.py --config /path/to/wireguard.conf
"""

import json
import zlib
import base64
import argparse
from typing import Dict, Any


def parse_wireguard_config(config_text: str) -> Dict[str, Any]:
    """
    Парсит WireGuard конфигурацию в словарь.
    
    Args:
        config_text: Текст конфигурации WireGuard
        
    Returns:
        Словарь с параметрами конфигурации
    """
    config = {
        'interface': {},
        'peer': {}
    }
    
    current_section = None
    
    for line in config_text.split('\n'):
        line = line.strip()
        
        # Пропускаем пустые строки и комментарии
        if not line or line.startswith('#'):
            continue
            
        # Определяем секцию
        if line.startswith('['):
            section_name = line.strip('[]').lower()
            current_section = section_name
            continue
        
        # Парсим параметры
        if '=' in line and current_section:
            key, value = line.split('=', 1)
            key = key.strip()
            value = value.strip()
            
            if current_section == 'interface':
                config['interface'][key] = value
            elif current_section == 'peer':
                config['peer'][key] = value
    
    return config


def wireguard_to_vpn_link(config: Dict[str, Any]) -> str:
    """
    Конвертирует WireGuard конфигурацию в vpn:// ссылку Amnezia.
    
    Args:
        config: Словарь с параметрами WireGuard конфигурации
        
    Returns:
        Строка с vpn:// ссылкой
    """
    interface = config['interface']
    peer = config['peer']
    
    # Извлекаем параметры из конфигурации
    private_key = interface.get('PrivateKey', '')
    address = interface.get('Address', '10.8.1.2/24')
    dns_servers = interface.get('DNS', '1.1.1.1, 8.8.8.8')
    
    # Парсим DNS серверы
    dns_list = [dns.strip() for dns in dns_servers.split(',')]
    dns1 = dns_list[0] if len(dns_list) > 0 else '1.1.1.1'
    dns2 = dns_list[1] if len(dns_list) > 1 else '8.8.8.8'
    
    public_key = peer.get('PublicKey', '')
    preshared_key = peer.get('PresharedKey', '')
    endpoint = peer.get('Endpoint', ':51820')
    
    # AmneziaWG специфичные параметры
    jc = int(interface.get('Jc', 6))
    jmin = int(interface.get('Jmin', 10))
    jmax = int(interface.get('Jmax', 50))
    s1 = int(interface.get('S1', 123))
    s2 = int(interface.get('S2', 136))
    h1 = int(interface.get('H1', 1043813656))
    h2 = int(interface.get('H2', 1394807736))
    h3 = int(interface.get('H3', 850386757))
    h4 = int(interface.get('H4', 714960491))
    
    # Парсим endpoint
    if ':' in endpoint:
        host_name, port = endpoint.split(':', 1)
    else:
        host_name = endpoint
        port = '51820'
    
    # Определяем тип контейнера на основе наличия AmneziaWG параметров
    has_awg_params = any(key in interface for key in ['Jc', 'Jmin', 'Jmax', 'S1', 'S2', 'H1', 'H2', 'H3', 'H4'])
    container_type = "amnezia-awg" if has_awg_params else "amnezia-wg"
    protocol_key = "awg" if has_awg_params else "wireguard"
    
    # Создаем конфигурацию протокола
    protocol_config = {
        "port": port,
        "client_priv_key": private_key,
        "client_ip": address,
        "server_pub_key": public_key,
    }
    
    # Добавляем preshared key если есть
    if preshared_key:
        protocol_config["psk_key"] = preshared_key
    
    # Добавляем AmneziaWG параметры если есть
    if has_awg_params:
        protocol_config.update({
            "Jc": jc,
            "Jmin": jmin,
            "Jmax": jmax,
            "S1": s1,
            "S2": s2,
            "H1": str(h1),
            "H2": str(h2),
            "H3": str(h3),
            "H4": str(h4)
        })
    
    # Создаем JSON структуру Amnezia
    server_config = {
        "hostName": host_name,
        "defaultContainer": container_type,
        "dns1": dns1,
        "dns2": dns2,
        "containers": [
            {
                "container": container_type,
                protocol_key: protocol_config
            }
        ]
    }
    
    # Сериализуем в JSON (компактный формат)
    json_data = json.dumps(server_config, separators=(',', ':')).encode('utf-8')
    
    # Сжимаем с zlib (уровень 8)
    compressed = zlib.compress(json_data, 8)
    
    # Кодируем в URL-safe Base64 без padding
    base64_encoded = base64.urlsafe_b64encode(compressed).decode('ascii').rstrip('=')
    
    return f"vpn://{base64_encoded}"


def decode_vpn_link(vpn_link: str) -> Dict[str, Any]:
    """
    Декодирует vpn:// ссылку обратно в JSON конфигурацию.
    
    Args:
        vpn_link: Строка с vpn:// ссылкой
        
    Returns:
        Словарь с конфигурацией
    """
    # Убираем префикс vpn://
    encoded = vpn_link.replace('vpn://', '')
    
    # Добавляем padding если нужно (для URL-safe base64)
    padding = 4 - (len(encoded) % 4)
    if padding != 4:
        encoded += '=' * padding
    
    # Декодируем из URL-safe Base64
    compressed = base64.urlsafe_b64decode(encoded)
    
    # Распаковываем zlib
    json_data = zlib.decompress(compressed)
    
    # Парсим JSON
    config = json.loads(json_data)
    
    return config


def main():
    """Основная функция."""
    parser = argparse.ArgumentParser(
        description='Конвертирует WireGuard конфигурацию в vpn:// ссылку формата Amnezia Client'
    )
    parser.add_argument(
        '--config',
        help='Путь к файлу с WireGuard конфигурацией',
        type=str
    )
    parser.add_argument(
        '--decode',
        help='Декодировать vpn:// ссылку обратно в JSON',
        type=str
    )
    
    args = parser.parse_args()
    
    if args.decode:
        # Режим декодирования
        print("Декодирование vpn:// ссылки...")
        print()
        try:
            config = decode_vpn_link(args.decode)
            print(json.dumps(config, indent=2, ensure_ascii=False))
        except Exception as e:
            print(f"Ошибка при декодировании vpn:// ссылки: {e}")
            print("Убедитесь, что ссылка корректна и начинается с 'vpn://'")
            return 1
        return
    
    # Режим кодирования
    if args.config:
        # Читаем конфигурацию из файла
        try:
            with open(args.config, 'r', encoding='utf-8') as f:
                config_text = f.read()
        except FileNotFoundError:
            print(f"Ошибка: Файл '{args.config}' не найден")
            return 1
        except PermissionError:
            print(f"Ошибка: Нет прав на чтение файла '{args.config}'")
            return 1
        except Exception as e:
            print(f"Ошибка при чтении файла: {e}")
            return 1
    else:
        # Используем пример конфигурации из проблемы
        config_text = """
[Interface]
PrivateKey = QI1ESrtAWzg4I6M8v8roRRdqldRCosjR6zpgFp1FRnM=
Address = 10.8.1.2/24
DNS = 1.1.1.1, 8.8.8.8
Jc = 6
Jmin = 10
Jmax = 50
S1 = 123
S2 = 136
H1 = 1043813656
H2 = 1394807736
H3 = 850386757
H4 = 714960491

[Peer]
PublicKey = ARATMWdjtitj3/MO8tCq7mMA7XL84SucUq+mKccNsTs=
PresharedKey = yaYGl/gM1vNml0ST+RWkAQnc3+eC9iZ9TPyz3jvuIFc=
Endpoint = 89.125.213.14:46811
AllowedIPs = 0.0.0.0/0
PersistentKeepalive = 25
"""
    
    # Парсим конфигурацию
    config = parse_wireguard_config(config_text)
    
    # Генерируем vpn:// ссылку
    vpn_link = wireguard_to_vpn_link(config)
    
    print("WireGuard конфигурация успешно преобразована в vpn:// ссылку!")
    print()
    print("vpn:// ссылка:")
    print(vpn_link)
    print()
    print("Вы можете использовать эту ссылку для:")
    print("1. Импорта в Amnezia Client")
    print("2. Генерации QR кода")
    print("3. Обмена конфигурацией с другими пользователями")
    print()
    print("Для декодирования обратно используйте:")
    print(f"  python3 {__file__} --decode '{vpn_link}'")


if __name__ == '__main__':
    main()
