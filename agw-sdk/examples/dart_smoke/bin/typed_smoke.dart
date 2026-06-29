// Dart-демо ТИПИЗИРОВАННЫХ методов C-ABI agw-sdk (Tier 2, Шаг 5) через dart:ffi.
//
// В отличие от smoke.dart (голый post), здесь биндинги к agw_get_services / agw_import_service:
// SDK сам собирает payload, шифрует, ходит на шлюз и отдаёт типизированный результат
// (JSON услуг; для import — распакованный конфиг или капчу). Картинку капчи рисует приложение.
//
// Переменные окружения (все опциональны):
//   AGW_GATEWAY      хост шлюза с "%1"-подстановкой, напр. "http://gw.dev.amzsvc.com:80/"
//   AGW_PUBKEY_FILE  путь к PEM ПУБЛИЧНОГО ключа шлюза (по умолчанию — тестовый фикстур)
//   AGW_DEV          "1" → dev-режим
//   AGW_IMPORT_PUBKEY  если задан — дополнительно дёрнуть agw_import_service с этим public_key
//   AGW_SERVICE_TYPE / AGW_SERVICE_PROTOCOL  для import (по умолчанию amnezia-premium / awg)
//   AGW_CAPI_LIB     путь к libagw_capi.* (иначе ../../build-local)

import 'dart:ffi';
import 'dart:io';

import 'package:ffi/ffi.dart';

// --- зеркала структур C-ABI (см. include/agw/c_abi.h) ------------------------

final class AgwConfig extends Struct {
  external Pointer<Utf8> gatewayEndpoint;
  external Pointer<Utf8> agwPublicKeyPem;
  external Pointer<Pointer<Utf8>> s3Primary;
  @Size()
  external int s3PrimaryCount;
  external Pointer<Pointer<Utf8>> s3Fallback;
  @Size()
  external int s3FallbackCount;
  @Int32()
  external int isDevEnvironment;
  @Int32()
  external int requestTimeoutMsecs;
  @Int32()
  external int proxyHealthTimeoutMsecs;
  @Int32()
  external int proxyStorageTimeoutMsecs;
  @Int32()
  external int threadPoolSize;
  external Pointer<Void> onBeforeRequest;
  external Pointer<Void> onBeforeRequestUserData;
  external Pointer<Void> log;
  external Pointer<Void> logUserData;
}

final class AgwGatewayRequest extends Struct {
  external Pointer<Utf8> osVersion;
  external Pointer<Utf8> appVersion;
  external Pointer<Utf8> appLanguage;
  external Pointer<Utf8> installationUuid;
  external Pointer<Utf8> userCountryCode;
  external Pointer<Utf8> serverCountryCode;
  external Pointer<Utf8> serviceType;
  external Pointer<Utf8> serviceProtocol;
  external Pointer<Utf8> authDataJson;
}

final class AgwJsonResult extends Struct {
  @Int32()
  external int error;
  external Pointer<Utf8> json;
  @Size()
  external int jsonLen;
}

final class AgwImportResult extends Struct {
  @Int32()
  external int error;
  @Int32()
  external int captchaRequired;
  external Pointer<Utf8> captchaId;
  external Pointer<Utf8> captchaImage;
  external Pointer<Utf8> captchaHint;
  external Pointer<Utf8> serverConfigJson;
}

typedef _CreateC = Pointer<Void> Function(Pointer<AgwConfig>);
typedef _DestroyC = Void Function(Pointer<Void>);
typedef _DestroyDart = void Function(Pointer<Void>);

typedef _GetServicesC = AgwJsonResult Function(
    Pointer<Void>, Pointer<Utf8>, Pointer<Utf8>, Pointer<Utf8>, Pointer<Utf8>);
typedef _JsonFreeC = Void Function(Pointer<AgwJsonResult>);
typedef _JsonFreeDart = void Function(Pointer<AgwJsonResult>);

typedef _ImportServiceC = AgwImportResult Function(
    Pointer<Void>, Pointer<AgwGatewayRequest>, Pointer<Utf8>);
typedef _ImportFreeC = Void Function(Pointer<AgwImportResult>);
typedef _ImportFreeDart = void Function(Pointer<AgwImportResult>);

String _libPath() {
  final env = Platform.environment['AGW_CAPI_LIB'];
  if (env != null) return env;
  final base = '${Directory.current.path}/../../build-local';
  if (Platform.isMacOS) return '$base/libagw_capi.dylib';
  if (Platform.isWindows) return '$base/agw_capi.dll';
  return '$base/libagw_capi.so';
}

String _defaultPubKey() {
  final f = File('${Directory.current.path}/../../tests/golden/fixtures/test_rsa_pub.pem');
  return f.existsSync() ? f.readAsStringSync() : 'not a real pem key';
}

String _clip(String s) => s.length > 300 ? '${s.substring(0, 300)}…' : s;

int main() {
  final env = Platform.environment;
  final lib = DynamicLibrary.open(_libPath());
  final create = lib.lookupFunction<_CreateC, _CreateC>('agw_client_create');
  final destroy = lib.lookupFunction<_DestroyC, _DestroyDart>('agw_client_destroy');
  final getServices = lib.lookupFunction<_GetServicesC, _GetServicesC>('agw_get_services');
  final jsonFree = lib.lookupFunction<_JsonFreeC, _JsonFreeDart>('agw_json_result_free');
  final importService = lib.lookupFunction<_ImportServiceC, _ImportServiceC>('agw_import_service');
  final importFree = lib.lookupFunction<_ImportFreeC, _ImportFreeDart>('agw_import_result_free');

  final gateway = env['AGW_GATEWAY'] ?? 'http://gw.example.test/';
  final pubKeyFile = env['AGW_PUBKEY_FILE'];
  final pubKey = pubKeyFile != null ? File(pubKeyFile).readAsStringSync() : _defaultPubKey();
  final isDev = (env['AGW_DEV'] == '1') ? 1 : 0;

  stdout.writeln('=== agw-sdk Dart TYPED demo ===');
  stdout.writeln('gateway=$gateway dev=$isDev pubkey=${pubKeyFile ?? "(test fixture)"}');

  // конфиг клиента
  final cfg = calloc<AgwConfig>();
  final gw = gateway.toNativeUtf8();
  final pk = pubKey.toNativeUtf8();
  cfg.ref.gatewayEndpoint = gw;
  cfg.ref.agwPublicKeyPem = pk;
  cfg.ref.requestTimeoutMsecs = 8000;
  cfg.ref.isDevEnvironment = isDev;
  final client = create(cfg);

  // ---------- agw_get_services (типизированный JSON-результат) ----------
  stdout.writeln('\n--- agw_get_services ---');
  final os = 'macos'.toNativeUtf8();
  final appv = '4.9.0'.toNativeUtf8();
  final cli = 'amnezia'.toNativeUtf8();
  final lang = 'en'.toNativeUtf8();
  final sres = getServices(client, os, appv, cli, lang);
  stdout.writeln('   error=${sres.error} jsonLen=${sres.jsonLen}');
  if (sres.json != nullptr && sres.jsonLen > 0) {
    stdout.writeln('   services=${_clip(sres.json.toDartString(length: sres.jsonLen))}');
  }
  final sp = calloc<AgwJsonResult>()
    ..ref.error = sres.error
    ..ref.json = sres.json
    ..ref.jsonLen = sres.jsonLen;
  jsonFree(sp);
  calloc.free(sp);
  calloc.free(os);
  calloc.free(appv);
  calloc.free(cli);
  calloc.free(lang);

  // ---------- agw_import_service (опционально, нужен public_key) ----------
  final importPubKey = env['AGW_IMPORT_PUBKEY'];
  if (importPubKey != null && importPubKey.isNotEmpty) {
    stdout.writeln('\n--- agw_import_service ---');
    final req = calloc<AgwGatewayRequest>();
    final allocs = <Pointer<NativeType>>[];
    Pointer<Utf8> s(String v) {
      final p = v.toNativeUtf8();
      allocs.add(p);
      return p;
    }

    req.ref.osVersion = s('macos');
    req.ref.appVersion = s('4.9.0');
    req.ref.appLanguage = s('en');
    req.ref.installationUuid = s('00000000-0000-0000-0000-000000000000');
    req.ref.serviceType = s(env['AGW_SERVICE_TYPE'] ?? 'amnezia-premium');
    req.ref.serviceProtocol = s(env['AGW_SERVICE_PROTOCOL'] ?? 'awg');
    final pkc = importPubKey.toNativeUtf8();

    final ires = importService(client, req, pkc);
    stdout.writeln('   error=${ires.error} captchaRequired=${ires.captchaRequired}');
    if (ires.captchaRequired != 0) {
      stdout.writeln('   captchaId=${ires.captchaId == nullptr ? "" : ires.captchaId.toDartString()} '
          '(картинку рисует приложение)');
    } else if (ires.serverConfigJson != nullptr) {
      stdout.writeln('   config=${_clip(ires.serverConfigJson.toDartString())}');
    }
    final ip = calloc<AgwImportResult>()
      ..ref.error = ires.error
      ..ref.captchaRequired = ires.captchaRequired
      ..ref.captchaId = ires.captchaId
      ..ref.captchaImage = ires.captchaImage
      ..ref.captchaHint = ires.captchaHint
      ..ref.serverConfigJson = ires.serverConfigJson;
    importFree(ip);
    calloc.free(ip);
    calloc.free(pkc);
    for (final p in allocs) {
      calloc.free(p);
    }
    calloc.free(req);
  }

  destroy(client);
  calloc.free(gw);
  calloc.free(pk);
  calloc.free(cfg);
  stdout.writeln('\n=== done ===');
  return 0;
}
