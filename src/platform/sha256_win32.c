#ifdef _WIN32
#include "platform/sha256.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wincrypt.h>

int platform_sha256(const void *const *parts, const size_t *sizes, size_t count,
                    uint8_t digest[PLATFORM_SHA256_SIZE])
{
    HCRYPTPROV provider = 0;
    HCRYPTHASH hash = 0;
    DWORD size = PLATFORM_SHA256_SIZE;
    size_t i;
    int ok = 0;

    if (!CryptAcquireContextA(&provider, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT) ||
        !CryptCreateHash(provider, CALG_SHA_256, 0, 0, &hash))
        goto done;
    for (i = 0; i < count; ++i)
        if (sizes[i] > MAXDWORD || !CryptHashData(hash, parts[i], (DWORD)sizes[i], 0))
            goto done;
    ok = CryptGetHashParam(hash, HP_HASHVAL, digest, &size, 0) && size == PLATFORM_SHA256_SIZE;
done:
    if (hash) CryptDestroyHash(hash);
    if (provider) CryptReleaseContext(provider, 0);
    return ok;
}
#endif
