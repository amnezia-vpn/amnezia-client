/*
 * tvOS does not export these iOS runtime helpers used by Go cgo archives.
 * WireGuardKitGo references them indirectly; provide no-op stubs for tvOS.
 */
void darwin_arm_init_mach_exception_handler(void) {}
void darwin_arm_init_thread_exception_port(void) {}
