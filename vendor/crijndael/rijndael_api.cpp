#include "core/rijndael_api.h"
#include "Rijndael.h"

int rijndael_encrypt(const unsigned char *key, size_t bytes,
                     const unsigned char *input, unsigned char *output)
{
    if (!key || !input || !output || (bytes != 16 && bytes != 24 && bytes != 32))
        return 0;
    try {
        CRijndael cipher;
        const char chain[32] = {};
        char encrypted[32];

        cipher.MakeKey(reinterpret_cast<const char *>(key), chain,
                       static_cast<int>(bytes), static_cast<int>(bytes));
        cipher.EncryptBlock(reinterpret_cast<const char *>(input), encrypted);
        std::memcpy(output, encrypted, bytes);
        return 1;
    } catch (...) {
        return 0;
    }
}
