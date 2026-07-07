#include "persist_blob.h"

bool persist_blob_write(uint32_t base_key, const void* data, size_t size)
{
    if(size > PERSIST_BLOB_MAX_CHUNKS * PERSIST_DATA_MAX_LENGTH)
    {
        APP_LOG(APP_LOG_LEVEL_ERROR, "blob %d: %d B exceeds the %d-chunk cap",
            (int)base_key, (int)size, PERSIST_BLOB_MAX_CHUNKS);
        return false;
    }

    const uint8_t* bytes = (const uint8_t*)data;
    size_t offset = 0;
    uint32_t chunk = 0;
    bool ok = true;
    while(offset < size)
    {
        size_t chunk_size = size - offset;
        if(chunk_size > PERSIST_DATA_MAX_LENGTH)
        {
            chunk_size = PERSIST_DATA_MAX_LENGTH;
        }
        int written = persist_write_data(base_key + chunk, &bytes[offset], chunk_size);
        if(written != (int)chunk_size)
        {
            APP_LOG(APP_LOG_LEVEL_ERROR, "blob %d chunk %d: wrote %d of %d B",
                (int)base_key, (int)chunk, written, (int)chunk_size);
            ok = false;
        }
        offset += chunk_size;
        chunk++;
    }

    // Drop stale chunks left over from a previously larger blob.
    while(chunk < PERSIST_BLOB_MAX_CHUNKS && persist_exists(base_key + chunk))
    {
        persist_delete(base_key + chunk);
        chunk++;
    }

    return ok;
}

size_t persist_blob_read(uint32_t base_key, void* out, size_t size)
{
    uint8_t* bytes = (uint8_t*)out;
    size_t offset = 0;
    uint32_t chunk = 0;
    while(offset < size && chunk < PERSIST_BLOB_MAX_CHUNKS && persist_exists(base_key + chunk))
    {
        size_t want = size - offset;
        if(want > PERSIST_DATA_MAX_LENGTH)
        {
            want = PERSIST_DATA_MAX_LENGTH;
        }
        int read = persist_read_data(base_key + chunk, &bytes[offset], want);
        if(read <= 0)
        {
            break;
        }
        offset += (size_t)read;
        chunk++;
    }
    return offset;
}

size_t persist_blob_size(uint32_t base_key)
{
    size_t total = 0;
    for(uint32_t chunk = 0; chunk < PERSIST_BLOB_MAX_CHUNKS && persist_exists(base_key + chunk); chunk++)
    {
        total += (size_t)persist_get_size(base_key + chunk);
    }
    return total;
}

void persist_blob_delete(uint32_t base_key)
{
    for(uint32_t chunk = 0; chunk < PERSIST_BLOB_MAX_CHUNKS && persist_exists(base_key + chunk); chunk++)
    {
        persist_delete(base_key + chunk);
    }
}
