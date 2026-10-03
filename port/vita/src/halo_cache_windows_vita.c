/* Preserve original map precache owners while native logical resource I/O
 * stays in vita_cache_bridge.c. Xbox DVD request queues do not own Vita reads. */
#define cache_file_read halo_vita_dvd_cache_file_read
#define cache_file_promote_read halo_vita_dvd_cache_file_promote_read
#define cache_file_block_until_not_busy halo_vita_dvd_cache_file_block_until_not_busy
#include "../../../source/cache/cache_files_windows.c"
#undef cache_file_read
#undef cache_file_promote_read
#undef cache_file_block_until_not_busy
