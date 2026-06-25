// Dart smoke для C-ABI agw-sdk: создать клиент + post через dart:ffi.
// Детерминированный путь без сети: невалидный публичный ключ → ApiMissingAgwPublicKey (1105).
//
// Запуск:
//   dart pub get
//   dart run   (библиотека ищется в ../../build-local или по AGW_CAPI_LIB)

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
typedef _FreeC = Void Function(Pointer<AgwResponse>);
typedef _FreeDart = void Function(Pointer<AgwResponse>);
typedef _DestroyC = Void Function(Pointer<Void>);
typedef _DestroyDart = void Function(Pointer<Void>);

String _libPath() {
  final env = Platform.environment['AGW_CAPI_LIB'];
  if (env != null) return env;
  final base = '${Directory.current.path}/../../build-local';
  if (Platform.isMacOS) return '$base/libagw_capi.dylib';
  if (Platform.isWindows) return '$base/agw_capi.dll';
  return '$base/libagw_capi.so';
}

int main() {
  final lib = DynamicLibrary.open(_libPath());

  final create = lib.lookupFunction<_CreateC, _CreateC>('agw_client_create');
  final post = lib.lookupFunction<_PostC, _PostC>('agw_client_post');
  final free = lib.lookupFunction<_FreeC, _FreeDart>('agw_response_free');
  final destroy = lib.lookupFunction<_DestroyC, _DestroyDart>('agw_client_destroy');

  final cfg = calloc<AgwConfig>();
  cfg.ref.gatewayEndpoint = 'gw.example.test'.toNativeUtf8();
  cfg.ref.agwPublicKeyPem = 'not a real pem key'.toNativeUtf8(); // → 1105, без сети
  cfg.ref.requestTimeoutMsecs = 5000;

  final client = create(cfg);
  if (client == nullptr) {
    stderr.writeln('FAIL: agw_client_create returned null');
    return 1;
  }

  final endpoint = 'https://%1/api/v1/test'.toNativeUtf8();
  final payload = '{"x":1}'.toNativeUtf8();
  final empty = ''.toNativeUtf8();

  final resp = post(client, endpoint, payload, empty, empty, nullptr);
  stdout.writeln('post error code = ${resp.error}');
  final ok = resp.error == 1105; // ApiMissingAgwPublicKey

  // освободить тело ответа
  final respPtr = calloc<AgwResponse>();
  respPtr.ref.error = resp.error;
  respPtr.ref.body = resp.body;
  respPtr.ref.bodyLen = resp.bodyLen;
  free(respPtr);
  calloc.free(respPtr);

  destroy(client);

  calloc.free(cfg.ref.gatewayEndpoint);
  calloc.free(cfg.ref.agwPublicKeyPem);
  calloc.free(endpoint);
  calloc.free(payload);
  calloc.free(empty);
  calloc.free(cfg);

  stdout.writeln(ok ? 'OK' : 'FAIL');
  return ok ? 0 : 1;
}
