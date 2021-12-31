#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/**
 * No error.
 */
#define ERR_OK 0

/**
 * Config path error.
 */
#define ERR_CONFIG_PATH 1

/**
 * Config parsing error.
 */
#define ERR_CONFIG 2

/**
 * IO error.
 */
#define ERR_IO 3

/**
 * Config file watcher error.
 */
#define ERR_WATCHER 4

/**
 * Async channel send error.
 */
#define ERR_ASYNC_CHANNEL_SEND 5

/**
 * Sync channel receive error.
 */
#define ERR_SYNC_CHANNEL_RECV 6

/**
 * Runtime manager error.
 */
#define ERR_RUNTIME_MANAGER 7

/**
 * No associated config file.
 */
#define ERR_NO_CONFIG_FILE 8

/**
 * Starts leaf with options, on a successful start this function blocks the current
 * thread.
 *
 * @note This is not a stable API, parameters will change from time to time.
 *
 * @param rt_id A unique ID to associate this leaf instance, this is required when
 *              calling subsequent FFI functions, e.g. reload, shutdown.
 * @param config_path The path of the config file, must be a file with suffix .conf
 *                    or .json, according to the enabled features.
 * @param auto_reload Enabls auto reloading when config file changes are detected,
 *                    takes effect only when the "auto-reload" feature is enabled.
 * @param multi_thread Whether to use a multi-threaded runtime.
 * @param auto_threads Sets the number of runtime worker threads automatically,
 *                     takes effect only when multi_thread is true.
 * @param threads Sets the number of runtime worker threads, takes effect when
 *                     multi_thread is true, but can be overridden by auto_threads.
 * @param stack_size Sets stack size of the runtime worker threads, takes effect when
 *                   multi_thread is true.
 * @return ERR_OK on finish running, any other errors means a startup failure.
 */
int32_t leaf_run_with_options(uint16_t rt_id,
                              const char *config_path,
                              bool auto_reload,
                              bool multi_thread,
                              bool auto_threads,
                              int32_t threads,
                              int32_t stack_size);

/**
 * Starts leaf with a single-threaded runtime, on a successful start this function
 * blocks the current thread.
 *
 * @param rt_id A unique ID to associate this leaf instance, this is required when
 *              calling subsequent FFI functions, e.g. reload, shutdown.
 * @param config_path The path of the config file, must be a file with suffix .conf
 *                    or .json, according to the enabled features.
 * @return ERR_OK on finish running, any other errors means a startup failure.
 */
int32_t leaf_run(uint16_t rt_id, const char *config_path);

/**
 * Reloads DNS servers, outbounds and routing rules from the config file.
 *
 * @param rt_id The ID of the leaf instance to reload.
 *
 * @return Returns ERR_OK on success.
 */
int32_t leaf_reload(uint16_t rt_id);

/**
 * Shuts down leaf.
 *
 * @param rt_id The ID of the leaf instance to reload.
 *
 * @return Returns true on success, false otherwise.
 */
bool leaf_shutdown(uint16_t rt_id);

/**
 * Tests the configuration.
 *
 * @param config_path The path of the config file, must be a file with suffix .conf
 *                    or .json, according to the enabled features.
 * @return Returns ERR_OK on success, i.e no syntax error.
 */
int32_t leaf_test_config(const char *config_path);
