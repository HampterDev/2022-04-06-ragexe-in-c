#ifndef GRF_FIXTURE_H
#define GRF_FIXTURE_H

enum { GRF_FIXTURE_COUNT = 40, GRF_FIXTURE_PAYLOAD = 384 };
static unsigned char archive_bytes[65536];
static unsigned archive_size, archive_table;
static unsigned fixture_name_offsets[GRF_FIXTURE_COUNT];
static unsigned fixture_record_offsets[GRF_FIXTURE_COUNT];
static unsigned char table_bytes[32768];
static unsigned table_size;

static void store32(unsigned char *p, unsigned value) { memcpy(p, &value, 4); }
static BYTE fixture_des_encode(struct Dec *self, const BYTE *src, int len, BYTE *dst, int *outLen, int cycle)
{
    int i, copied, padded;
    padded = (len + 7) / 8 * 8;
    *outLen = (padded + 7) / 8 * 8;
    cycle = des_cycle(cycle);
    copied = 0;
    for (i = 0;; i++) {
        const BYTE *in = src + i * 8;
        BYTE *out = dst + i * 8;
        if (i < 20 || i % cycle == 0) {
            DWORD block[2], lo, hi;
            memcpy(&lo, in, 4);
            memcpy(&hi, in + 4, 4);
            des_crypt_block(1, self, lo, hi, block, 0);
            memcpy(out, block, 8);
        } else if (copied != 7) {
            copied++;
            memcpy(out, in, 8);
        } else {
            out[3] = in[0];
            out[4] = in[1];
            out[0] = in[3];
            out[1] = in[4];
            out[2] = in[5];
            out[5] = in[6];
            out[6] = in[2];
            out[7] = shuffle_byte(in[7]);
            copied = 1;
        }
        if (i * 8 + 8 >= padded)
            break;
    }
    return 1;
}
static void fixture_name(unsigned index, char name[252])
{
    if (index % 11 == 0) strcpy(name, "data\\duplicate.gat");
    else if (index == 1) strcpy(name, "data\\한글\\모델.gnd");
    else sprintf(name, "data\\item%02u.%s", (index * 17) % 43, index % 2 ? "bin" : "str");
}
static void make_grf_fixture(unsigned version)
{
    unsigned i, payload_end = 0x2e;
    unsigned char raw[GRF_FIXTURE_PAYLOAD], compressed[512], encoded[512];
    struct Dec cipher;
    BYTE key[8] = {0};
    dec_construct(&cipher, key);
    memset(archive_bytes, 0, sizeof archive_bytes);
    memset(table_bytes, 0, sizeof table_bytes);
    memcpy(archive_bytes, "Master of Magic", 16);
    store32(archive_bytes + 0x22, 13);
    store32(archive_bytes + 0x26, GRF_FIXTURE_COUNT + 20);
    store32(archive_bytes + 0x2a, version);
    table_size = 0;
    for (i = 0; i < GRF_FIXTURE_COUNT; ++i) {
        char name[252];
        unsigned j, aligned, packed, flags, offset = payload_end - 0x2e;
        unsigned long compressed_size = sizeof compressed;
        int out_size;
        fixture_name(i, name);
        for (j = 0; j < sizeof raw; ++j) raw[j] = (BYTE)((j * 73 + i * 19) ^ (j >> 2));
        memset(compressed, 0, sizeof compressed);
        assert(compress(compressed, &compressed_size, raw, sizeof raw) == Z_OK);
        packed = (unsigned)compressed_size;
        flags = GRF_FLAG_DEFLATED;
        if (version < 0x200) flags |= !(i % 2) || i == 1 || i % 11 == 0 ? GRF_FLAG_DES_HEADER : GRF_FLAG_DES;
        else if (i % 4 == 1) flags |= GRF_FLAG_DES_HEADER;
        else if (i % 4 == 2) flags |= GRF_FLAG_DES;
        else if (i % 4 == 3) { flags = 0; packed = sizeof raw; memcpy(compressed, raw, sizeof raw); }
        aligned = (packed + 7) & ~7u;
        if (flags & GRF_FLAG_DES) {
            unsigned n = packed; int digits = 0;
            do { ++digits; n /= 10; } while (n);
            assert(fixture_des_encode(&cipher, compressed, (int)aligned, encoded, &out_size, digits));
        } else if (flags & GRF_FLAG_DES_HEADER) {
            assert(dec_decode_header(&cipher, compressed, (int)aligned, encoded, &out_size));
        } else memcpy(encoded, compressed, aligned);
        memcpy(archive_bytes + payload_end, encoded, aligned); payload_end += aligned;
        fixture_name_offsets[i] = table_size;
        if (version >= 0x200) {
            unsigned n = (unsigned)strlen(name) + 1;
            memcpy(table_bytes + table_size, name, n); table_size += n;
        } else if (version == 0x100) {
            unsigned n = (unsigned)strlen(name) + 1;
            store32(table_bytes + table_size, n); table_size += 4;
            for (j = 0; j < n; ++j) table_bytes[table_size++] = (BYTE)(((BYTE)name[j] << 4) | ((BYTE)name[j] >> 4));
        } else {
            unsigned n = ((unsigned)strlen(name) + 8) & ~7u;
            BYTE plain[256] = {0}, cipher_name[256];
            memcpy(plain, name, strlen(name) + 1);
            assert(fixture_des_encode(&cipher, plain, (int)n, cipher_name, &out_size, 1));
            store32(table_bytes + table_size, n + 6); table_size += 6;
            for (j = 0; j < n; ++j) table_bytes[table_size++] = (BYTE)((cipher_name[j] << 4) | (cipher_name[j] >> 4));
            table_size += 4;
        }
        fixture_record_offsets[i] = table_size;
        store32(table_bytes + table_size, packed + (version < 0x200 ? sizeof raw + 0x2cb : 0));
        store32(table_bytes + table_size + 4, aligned + (version < 0x200 ? 0x92cb : 0));
        store32(table_bytes + table_size + 8, sizeof raw);
        table_bytes[table_size + 12] = (BYTE)(version < 0x200 ? flags & GRF_FLAG_DEFLATED : flags);
        store32(table_bytes + table_size + 13, offset); table_size += 17;
    }
    archive_table = payload_end;
    store32(archive_bytes + 0x1e, archive_table - 0x2e);
    if (version >= 0x200) {
        unsigned long packed = sizeof archive_bytes - payload_end - 8;
        assert(compress(archive_bytes + payload_end + 8, &packed, table_bytes, table_size) == Z_OK);
        store32(archive_bytes + payload_end, (unsigned)packed);
        store32(archive_bytes + payload_end + 4, table_size);
        archive_size = payload_end + 8 + (unsigned)packed;
    } else {
        memcpy(archive_bytes + payload_end, table_bytes, table_size);
        archive_size = payload_end + table_size;
    }
    dec_destruct(&cipher);
}
#endif
