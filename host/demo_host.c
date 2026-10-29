/*
 * SPDX-FileCopyrightText: 2026 EsyProject contributors
 * SPDX-License-Identifier: Apache-2.0
 */
#include "demo_security.h"
#include <openssl/bn.h>
#include <openssl/core_names.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define HEADER_BYTES 112u
#define META_BYTES 256u
#define CHUNK_BYTES 256u
#define SLOT_BYTES 0x60000u
#define SLOT_A 0x08040000u
#define SLOT_B 0x080A0000u
#define PICO_SLOT_A 0x10040000u
#define PICO_SLOT_B 0x100A0000u
#define IO_TIMEOUT_MS 30000

static uint32_t read_le(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
         ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static void write_le(uint8_t *p, uint32_t v) {
  for (unsigned i = 0; i < 4; ++i) p[i] = (uint8_t)(v >> (8u * i));
}
static uint32_t crc32(const uint8_t *p, size_t size) {
  uint32_t crc = UINT32_MAX;
  for (size_t i = 0; i < size; ++i) {
    crc ^= p[i];
    for (unsigned j = 0; j < 8; ++j)
      crc = (crc >> 1) ^ ((crc & 1u) ? 0xEDB88320u : 0u);
  }
  return ~crc;
}
static bool hash(const void *p, size_t size, uint8_t out[32]) {
  unsigned len = 0;
  return EVP_Digest(p, size, out, &len, EVP_sha256(), NULL) == 1 && len == 32u;
}
static EVP_PKEY *load_key(const char *path) {
  FILE *f = fopen(path, "rb");
  if (f == NULL) return NULL;
  EVP_PKEY *key = PEM_read_PrivateKey(f, NULL, NULL, NULL);
  fclose(f);
  char group[80] = {0};
  if (key == NULL || !EVP_PKEY_is_a(key, "EC") ||
      EVP_PKEY_get_group_name(key, group, sizeof(group), NULL) != 1 ||
      strcmp(group, "prime256v1") != 0) {
    EVP_PKEY_free(key);
    return NULL;
  }
  return key;
}
static bool sign_header(EVP_PKEY *key, uint8_t h[HEADER_BYTES]) {
  EVP_MD_CTX *ctx = EVP_MD_CTX_new();
  uint8_t der[80];
  size_t len = sizeof(der);
  bool ok = ctx != NULL &&
      EVP_DigestSignInit(ctx, NULL, EVP_sha256(), NULL, key) == 1 &&
      EVP_DigestSign(ctx, der, &len, h + 4, 40u) == 1;
  EVP_MD_CTX_free(ctx);
  if (!ok) return false;
  const unsigned char *p = der;
  ECDSA_SIG *sig = d2i_ECDSA_SIG(NULL, &p, (long)len);
  if (sig == NULL) return false;
  const BIGNUM *r = NULL, *s = NULL;
  ECDSA_SIG_get0(sig, &r, &s);
  ok = BN_bn2binpad(r, h + 44, 32) == 32 &&
       BN_bn2binpad(s, h + 76, 32) == 32;
  ECDSA_SIG_free(sig);
  return ok;
}
static bool verify_header(const uint8_t h[HEADER_BYTES]) {
  uint8_t digest[32];
  return memcmp(h, "ESYU", 4) == 0 && read_le(h + 108) == 0u &&
      read_le(h + 4) != 0u &&
      read_le(h + 8) >= 8u && read_le(h + 8) <= SLOT_BYTES &&
      hash(h + 4, 40u, digest) && demo_verify_signature(NULL, digest, h + 44);
}
static uint8_t *load_file(const char *path, size_t limit, size_t *size) {
  FILE *f = fopen(path, "rb");
  if (f == NULL) return NULL;
  if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
  const long len = ftell(f);
  if (len <= 0 || (unsigned long)len > limit || fseek(f, 0, SEEK_SET) != 0) {
    fclose(f); return NULL;
  }
  uint8_t *p = malloc((size_t)len);
  bool ok = p != NULL && fread(p, 1, (size_t)len, f) == (size_t)len;
  fclose(f);
  if (!ok) { free(p); return NULL; }
  *size = (size_t)len;
  return p;
}
static bool save_file(const char *path, const void *a, size_t na,
                      const void *b, size_t nb) {
  FILE *f = fopen(path, "wb");
  if (f == NULL) return false;
  bool ok = fwrite(a, 1, na, f) == na &&
            (nb == 0 || fwrite(b, 1, nb, f) == nb);
  if (fclose(f) != 0) ok = false;
  return ok;
}
static uint32_t image_slot(const uint8_t *image, size_t size) {
  if (size < 8u) return 0u;
  const uint32_t sp = read_le(image), pc = read_le(image + 4);
  if ((sp & 7u) != 0u || sp <= 0x20000000u || (pc & 1u) == 0u) return 0u;
  const uint32_t address = pc & ~1u;
  if (sp <= 0x20020000u) {
    if (address >= SLOT_A && address < SLOT_A + size) return SLOT_A;
    if (address >= SLOT_B && address < SLOT_B + size) return SLOT_B;
  }
  if (sp <= 0x20082000u) {
    if (address >= PICO_SLOT_A && address < PICO_SLOT_A + size) return PICO_SLOT_A;
    if (address >= PICO_SLOT_B && address < PICO_SLOT_B + size) return PICO_SLOT_B;
  }
  return 0u;
}
static bool package_valid(const uint8_t *p, size_t size) {
  uint8_t digest[32];
  return size >= HEADER_BYTES + 8u && verify_header(p) &&
      read_le(p + 8) == size - HEADER_BYTES &&
      image_slot(p + HEADER_BYTES, size - HEADER_BYTES) != 0u &&
      hash(p + HEADER_BYTES, size - HEADER_BYTES, digest) &&
      memcmp(digest, p + 12, 32) == 0;
}
static int export_key(const char *path, const char *out) {
  EVP_PKEY *key = load_key(path);
  uint8_t public_key[65];
  size_t len = 0;
  bool ok = key != NULL && EVP_PKEY_get_octet_string_param(key,
      OSSL_PKEY_PARAM_PUB_KEY, public_key, sizeof(public_key), &len) == 1 &&
      len == sizeof(public_key) && public_key[0] == 4u;
  EVP_PKEY_free(key);
  if (!ok) return 1;
  FILE *f = fopen(out, "w");
  if (f == NULL) return 1;
  fprintf(f, "/*\n * SPDX-FileCopyrightText: 2026 EsyProject contributors\n"
             " * SPDX-License-Identifier: Apache-2.0\n */\n"
             "#ifndef DEMO_PUBLIC_KEY_H\n#define DEMO_PUBLIC_KEY_H\n"
             "#include <stdint.h>\n"
             "/* Public sample key. Its private counterpart is included for demonstration. */\n"
             "static const uint8_t k_demo_public_key[64] = {\n");
  for (size_t i = 1; i < sizeof(public_key); ++i)
    fprintf(f, "0x%02x%s%s", public_key[i], i == 64u ? "" : ",",
            i % 8u == 0 ? "\n" : " ");
  fprintf(f, "};\n#endif\n");
  return fclose(f) == 0 ? 0 : 1;
}
static int pack(const char *key_path, const char *image_path,
                const char *version_text, const char *prefix) {
  char *end = NULL;
  errno = 0;
  unsigned long version = strtoul(version_text, &end, 10);
  if (errno || *end || version == 0 || version > UINT32_MAX ||
      version_text[0] == '-') return 1;
  size_t size = 0;
  uint8_t *image = load_file(image_path, SLOT_BYTES, &size);
  EVP_PKEY *key = load_key(key_path);
  uint8_t h[HEADER_BYTES] = {0}, meta[META_BYTES] = {0};
  bool ok = image != NULL && key != NULL && image_slot(image, size) != 0u;
  memcpy(h, "ESYU", 4);
  write_le(h + 4, (uint32_t)version);
  write_le(h + 8, (uint32_t)size);
  ok = ok && hash(image, size, h + 12) && sign_header(key, h) && verify_header(h);
  EVP_PKEY_free(key);
  if (!ok) {
    fprintf(stderr, "Invalid image/key: use an A/B-linked image and the compiled-in P-256 key.\n");
    free(image); return 1;
  }
  write_le(meta, 0x4553594Du);
  meta[4] = 1u;
  meta[6] = 2u; /* Factory provisioning starts with a confirmed baseline image. */
  memcpy(meta + 8, h + 4, 104u);
  write_le(meta + 112, crc32(meta, sizeof(meta)));
  char package_path[4096], metadata_path[4096];
  int a = snprintf(package_path, sizeof(package_path), "%s.esy", prefix);
  int b = snprintf(metadata_path, sizeof(metadata_path), "%s.meta.bin", prefix);
  ok = a > 0 && (size_t)a < sizeof(package_path) && b > 0 &&
       (size_t)b < sizeof(metadata_path) &&
       save_file(package_path, h, sizeof(h), image, size) &&
       save_file(metadata_path, meta, sizeof(meta), NULL, 0);
  if (ok) printf("Packed version %lu, %zu bytes, linked slot %c.\n", version,
                  size, (image_slot(image, size) == SLOT_A ||
                         image_slot(image, size) == PICO_SLOT_A) ? 'A' : 'B');
  free(image);
  return ok ? 0 : 1;
}
static int64_t monotonic_ms(void) {
  struct timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return -1;
  return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}
static bool serial_io(int fd, void *buffer, size_t size, bool writing) {
  uint8_t *p = buffer;
  const int64_t start = monotonic_ms();
  if (start < 0) return false;
  const int64_t deadline = start + IO_TIMEOUT_MS;
  while (size > 0) {
    int64_t now = monotonic_ms();
    if (now < 0 || now >= deadline) return false;
    struct pollfd event = {.fd = fd, .events = writing ? POLLOUT : POLLIN};
    int status = poll(&event, 1, (int)(deadline - now));
    if (status < 0 && errno == EINTR) continue;
    if (status <= 0 || (event.revents & (POLLERR | POLLHUP | POLLNVAL))) return false;
    ssize_t n = writing ? write(fd, p, size) : read(fd, p, size);
    if (n < 0 && (errno == EINTR || errno == EAGAIN)) continue;
    if (n <= 0) return false;
    p += n;
    size -= (size_t)n;
  }
  return true;
}
static int serial_open(const char *path) {
  int fd = open(path, O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (fd < 0) return -1;
  struct termios t;
  if (tcgetattr(fd, &t) != 0) { close(fd); return -1; }
  t.c_iflag = 0; t.c_oflag = 0; t.c_lflag = 0;
  t.c_cflag = CS8 | CREAD | CLOCAL;
  t.c_cc[VMIN] = 0; t.c_cc[VTIME] = 0;
  if (cfsetispeed(&t, B115200) != 0 || cfsetospeed(&t, B115200) != 0 ||
      tcsetattr(fd, TCSANOW, &t) != 0 || tcflush(fd, TCIOFLUSH) != 0) {
    close(fd); return -1;
  }
  return fd;
}
static bool response(int fd, uint8_t expected) {
  uint8_t c = 0;
  if (!serial_io(fd, &c, 1u, false)) {
    fprintf(stderr, "Serial timeout/disconnect. Restart the complete upload.\n");
    return false;
  }
  if (c != expected) {
    fprintf(stderr, "Target rejected the request or response was lost (0x%02x).\n", c);
    return false;
  }
  return true;
}
static int upload(const char *port, const char *path) {
  size_t size = 0;
  uint8_t *p = load_file(path, HEADER_BYTES + SLOT_BYTES, &size);
  if (p == NULL || !package_valid(p, size)) {
    fprintf(stderr, "Package hash, signature, vector or length is invalid.\n");
    free(p); return 1;
  }
  int fd = serial_open(port);
  if (fd < 0) { perror(port); free(p); return 1; }
  bool ok = serial_io(fd, p, HEADER_BYTES, true) && response(fd, 'R');
  for (size_t offset = HEADER_BYTES; ok && offset < size;) {
    size_t chunk = size - offset < CHUNK_BYTES ? size - offset : CHUNK_BYTES;
    ok = serial_io(fd, p + offset, chunk, true) && response(fd, 'A');
    offset += chunk;
    if (ok) printf("\r%zu / %zu bytes", offset - HEADER_BYTES, size - HEADER_BYTES);
  }
  ok = ok && response(fd, 'D');
  printf("\n%s\n", ok ? "Image accepted. Target will reboot and confirm after startup checks." :
                          "Upload failed. Reconnect and restart; never resume a partial image.");
  close(fd); free(p);
  return ok ? 0 : 1;
}
static int info(const char *port) {
  int fd = serial_open(port);
  if (fd < 0) { perror(port); return 1; }
  uint8_t request[4] = {'E','S','Y','I'}, reply[16];
  bool ok = serial_io(fd, request, sizeof(request), true) &&
      serial_io(fd, reply, sizeof(reply), false) && memcmp(reply, "ESYI", 4) == 0;
  if (ok && read_le(reply + 4) > 1u) {
    fprintf(stderr, "Target returned an invalid running slot.\n");
    ok = false;
  }
  if (ok) printf("Slot %c, version %u, confirmation %d\n",
      read_le(reply + 4) == 0u ? 'A' : 'B', read_le(reply + 8), (int32_t)read_le(reply + 12));
  close(fd);
  return ok ? 0 : 1;
}
static int selftest(void) {
  EVP_PKEY *key = load_key(DEMO_TEST_KEY);
  uint8_t h[HEADER_BYTES] = {0};
  memcpy(h, "ESYU", 4); write_le(h + 4, 1u); write_le(h + 8, 256u);
  bool ok = key != NULL && hash("test image", 10u, h + 12) &&
            sign_header(key, h) && verify_header(h) &&
            crc32((const uint8_t *)"123456789", 9u) == 0xCBF43926u;
  h[12] ^= 1u;
  ok = ok && !verify_header(h);
  h[12] ^= 1u; h[44] ^= 1u;
  ok = ok && !verify_header(h);
  EVP_PKEY_free(key);
  puts(ok ? "OpenSSL signatures accepted by MCU verifier; corrupt hash/signature rejected; CRC passed." :
            "Signing self-test FAILED.");
  return ok ? 0 : 1;
}
int main(int argc, char **argv) {
  if (argc == 4 && strcmp(argv[1], "export-key") == 0) return export_key(argv[2], argv[3]);
  if (argc == 6 && strcmp(argv[1], "pack") == 0) return pack(argv[2], argv[3], argv[4], argv[5]);
  if (argc == 4 && strcmp(argv[1], "upload") == 0) return upload(argv[2], argv[3]);
  if (argc == 3 && strcmp(argv[1], "info") == 0) return info(argv[2]);
  if (argc == 3 && strcmp(argv[1], "verify") == 0) {
    size_t size = 0; uint8_t *p = load_file(argv[2], HEADER_BYTES + SLOT_BYTES, &size);
    bool ok = p != NULL && package_valid(p, size); free(p);
    puts(ok ? "Package verified." : "Invalid package."); return ok ? 0 : 1;
  }
  if (argc == 2 && strcmp(argv[1], "selftest") == 0) return selftest();
  fprintf(stderr, "Usage:\n  demo_host pack KEY.pem IMAGE.bin VERSION OUTPUT_PREFIX\n"
      "  demo_host verify PACKAGE.esy\n  demo_host upload PORT PACKAGE.esy\n"
      "  demo_host info PORT\n  demo_host export-key KEY.pem OUTPUT.h\n  demo_host selftest\n");
  return 2;
}
