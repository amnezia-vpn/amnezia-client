// Dart-демо C-ABI agw-sdk через dart:ffi.
//
// Показывает поток запроса: подключает лог-хук SDK и onBeforeRequest, поэтому печатаются строки
// [agw] (post START -> direct request url -> direct response -> failover -> post DONE) — видно,
// что запрос ушёл и ответ пришёл. Делает синхронный post, а при AGW_ASYNC=1 — ещё и асинхронный
// (agw_client_post_async + NativeCallable.listener: коллбэк прилетает с потока пула SDK).
//
// Конфиг через переменные окружения (все опциональны):
//   AGW_GATEWAY      хост шлюза с "%1"-подстановкой, напр. "http://gw.dev.amzsvc.com:80/"
//   AGW_PUBKEY_FILE  путь к PEM ПУБЛИЧНОГО ключа шлюза (по умолчанию — тестовый фикстур)
//   AGW_S3_PRIMARY   список S3-адресов через запятую (failover)
//   AGW_DEV          "1" → dev-режим (S3-список открытым текстом)
//   AGW_ENDPOINT     шаблон пути, по умолчанию "%1v1/services"
//   AGW_PAYLOAD      JSON тела запроса
//   AGW_ASYNC        "1" → дополнительно прогнать асинхронный вызов
//   AGW_CAPI_LIB     путь к libagw_capi.* (иначе ../../build-local)

import 'dart:async';
import 'dart:ffi';
import 'dart:io';

import 'package:ffi/ffi.dart';

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

final class AgwResponse extends Struct {
  @Int32()
  external int error;
  external Pointer<Utf8> body;
  @Size()
  external int bodyLen;
}

typedef _CreateC = Pointer<Void> Function(Pointer<AgwConfig>);
typedef _PostC = AgwResponse Function(Pointer<Void>, Pointer<Utf8>, Pointer<Utf8>,
    Pointer<Utf8>, Pointer<Utf8>, Pointer<Void>);
typedef _PostAsyncC = Void Function(Pointer<Void>, Pointer<Utf8>, Pointer<Utf8>,
    Pointer<Utf8>, Pointer<Utf8>, Pointer<Void>, Pointer<Void>, Pointer<Void>);
typedef _PostAsyncDart = void Function(Pointer<Void>, Pointer<Utf8>, Pointer<Utf8>,
    Pointer<Utf8>, Pointer<Utf8>, Pointer<Void>, Pointer<Void>, Pointer<Void>);
typedef _FreeC = Void Function(Pointer<AgwResponse>);
typedef _FreeDart = void Function(Pointer<AgwResponse>);
typedef _DestroyC = Void Function(Pointer<Void>);
typedef _DestroyDart = void Function(Pointer<Void>);

typedef _LogNative = Void Function(Int32, Pointer<Utf8>, Pointer<Void>);
typedef _BeforeNative = Void Function(Pointer<Utf8>, Pointer<Void>);
typedef _PostCbNative = Void Function(AgwResponse, Pointer<Void>);

const _levels = ['DBG', 'INF', 'WRN', 'ERR'];

void _printLog(String tag, int level, Pointer<Utf8> message) {
  final lvl = (level >= 0 && level < _levels.length) ? _levels[level] : '?';
  stdout.writeln('   $tag [agw][$lvl] ${message.toDartString()}');
}

class _Cfg {
  final Pointer<AgwConfig> ptr;
  final List<Pointer<NativeType>> allocs;
  _Cfg(this.ptr, this.allocs);
  void free() {
    for (final p in allocs) {
      calloc.free(p);
    }
    calloc.free(ptr);
  }
}

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

late String gateway, pubKey, payload;
late int isDev;
late List<String> s3List;

_Cfg buildConfig(Pointer<Void> logFn, Pointer<Void> beforeFn) {
  final cfg = calloc<AgwConfig>();
  final allocs = <Pointer<NativeType>>[];

  final gw = gateway.toNativeUtf8();
  final pk = pubKey.toNativeUtf8();
  allocs.add(gw);
  allocs.add(pk);
  cfg.ref.gatewayEndpoint = gw;
  cfg.ref.agwPublicKeyPem = pk;
  cfg.ref.requestTimeoutMsecs = 8000;
  cfg.ref.isDevEnvironment = isDev;
  cfg.ref.onBeforeRequest = beforeFn;
  cfg.ref.log = logFn;

  if (s3List.isNotEmpty) {
    final arr = calloc<Pointer<Utf8>>(s3List.length);
    for (var i = 0; i < s3List.length; i++) {
      arr[i] = s3List[i].toNativeUtf8();
      allocs.add(arr[i]);
    }
    cfg.ref.s3Primary = arr;
    cfg.ref.s3PrimaryCount = s3List.length;
    allocs.add(arr);
  }
  return _Cfg(cfg, allocs);
}

Future<int> main() async {
  final env = Platform.environment;
  final lib = DynamicLibrary.open(_libPath());
  final create = lib.lookupFunction<_CreateC, _CreateC>('agw_client_create');
  final post = lib.lookupFunction<_PostC, _PostC>('agw_client_post');
  final postAsync = lib.lookupFunction<_PostAsyncC, _PostAsyncDart>('agw_client_post_async');
  final free = lib.lookupFunction<_FreeC, _FreeDart>('agw_response_free');
  final destroy = lib.lookupFunction<_DestroyC, _DestroyDart>('agw_client_destroy');

  gateway = env['AGW_GATEWAY'] ?? 'http://gw.example.test/';
  final pubKeyFile = env['AGW_PUBKEY_FILE'];
  pubKey = pubKeyFile != null ? File(pubKeyFile).readAsStringSync() : _defaultPubKey();
  final endpoint = env['AGW_ENDPOINT'] ?? '%1v1/services';
  payload = env['AGW_PAYLOAD'] ??
      '{"os_version":"macos","app_version":"4.9.0","cli_name":"amnezia","app_language":"en"}';
  isDev = (env['AGW_DEV'] == '1') ? 1 : 0;
  s3List = (env['AGW_S3_PRIMARY'] ?? '')
      .split(',')
      .map((s) => s.trim())
      .where((s) => s.isNotEmpty)
      .toList();

  stdout.writeln('=== agw-sdk Dart demo ===');
  stdout.writeln('gateway=$gateway  endpoint=$endpoint  dev=$isDev  s3primary=${s3List.length}');
  stdout.writeln('pubkey=${pubKeyFile ?? "(test fixture)"}');

  final endpointC = endpoint.toNativeUtf8();
  final payloadC = payload.toNativeUtf8();
  final svc = ''.toNativeUtf8();

  // ---------- SYNC (коллбэки isolateLocal: ядро sync исполняется на этом потоке) ----------
  stdout.writeln('\n--- SYNC post ---');
  final logSync = NativeCallable<_LogNative>.isolateLocal(
      (int lvl, Pointer<Utf8> m, Pointer<Void> _) => _printLog('[sync]', lvl, m));
  final beforeSync = NativeCallable<_BeforeNative>.isolateLocal(
      (Pointer<Utf8> h, Pointer<Void> _) =>
          stdout.writeln('   [sync] → onBeforeRequest host=${h.toDartString()}'));

  final cfgSync = buildConfig(logSync.nativeFunction.cast(), beforeSync.nativeFunction.cast());
  final clientSync = create(cfgSync.ptr);
  final resp = post(clientSync, endpointC, payloadC, svc, svc, nullptr);
  stdout.writeln('   [sync] RESULT errorCode=${resp.error} bodyLen=${resp.bodyLen}');
  if (resp.body != nullptr && resp.bodyLen > 0) {
    final body = resp.body.toDartString(length: resp.bodyLen);
    stdout.writeln('   [sync] body=${body.length > 200 ? "${body.substring(0, 200)}…" : body}');
  }
  final rp = calloc<AgwResponse>()
    ..ref.error = resp.error
    ..ref.body = resp.body
    ..ref.bodyLen = resp.bodyLen;
  free(rp);
  calloc.free(rp);
  destroy(clientSync);
  cfgSync.free();
  logSync.close();
  beforeSync.close();

  // ---------- ASYNC (коллбэки listener: прилетают с потока пула SDK) ----------
  if (env['AGW_ASYNC'] == '1') {
    stdout.writeln('\n--- ASYNC post (коллбэк с потока пула) ---');
    // ВАЖНО: лог-хук/onBeforeRequest сюда НЕ вешаем. Их const char* живут лишь во время вызова на
    // потоке пула, а NativeCallable.listener выполняется позже на Dart event-loop → указатель был бы
    // висячим. Result-коллбэк безопасен: body выделен в куче и принадлежит вызывающему (мы его освобождаем).
    final done = Completer<void>();

    final resultCb = NativeCallable<_PostCbNative>.listener((AgwResponse r, Pointer<Void> _) {
      stdout.writeln('   [async] CALLBACK (прилетел с потока пула) errorCode=${r.error} bodyLen=${r.bodyLen}');
      if (r.body != nullptr && r.bodyLen > 0) {
        final body = r.body.toDartString(length: r.bodyLen);
        stdout.writeln('   [async] body=${body.length > 200 ? "${body.substring(0, 200)}…" : body}');
      }
      final p = calloc<AgwResponse>()
        ..ref.error = r.error
        ..ref.body = r.body
        ..ref.bodyLen = r.bodyLen;
      free(p);
      calloc.free(p);
      done.complete();
    });

    final cfgAsync = buildConfig(nullptr, nullptr); // без лог/before-хуков (см. выше)
    final clientAsync = create(cfgAsync.ptr);
    stdout.writeln('   [async] post_async отправлен, ждём коллбэк…');
    postAsync(clientAsync, endpointC, payloadC, svc, svc, resultCb.nativeFunction.cast(),
        nullptr, nullptr);

    await done.future; // ждём, пока коллбэк прилетит с потока пула
    destroy(clientAsync);
    cfgAsync.free();
    resultCb.close();
  }

  calloc.free(endpointC);
  calloc.free(payloadC);
  calloc.free(svc);
  stdout.writeln('\n=== done ===');
  return 0;
}
