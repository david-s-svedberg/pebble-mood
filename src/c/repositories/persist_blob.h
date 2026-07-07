#pragma once

#include <pebble.h>

// Chunked persist storage. persist_write_data silently truncates anything over
// PERSIST_DATA_MAX_LENGTH (256 B), so blobs are split across consecutive keys:
// chunk i lives at base_key + i. Base keys must therefore be spaced at least
// PERSIST_BLOB_MAX_CHUNKS apart (see DataKeys in data.h).
#define PERSIST_BLOB_MAX_CHUNKS (32)

// Writes size bytes across chunks. Returns true when every chunk was written
// completely; logs and returns false otherwise (storage full / size over cap).
bool persist_blob_write(uint32_t base_key, const void* data, size_t size);

// Reads up to size bytes from the chunks. Returns the number of bytes read.
size_t persist_blob_read(uint32_t base_key, void* out, size_t size);

// Total bytes currently stored across the chunks.
size_t persist_blob_size(uint32_t base_key);

// Removes every stored chunk.
void persist_blob_delete(uint32_t base_key);
