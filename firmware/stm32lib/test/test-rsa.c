
#define USE_CUTEST

#ifdef USE_CUTEST
#  include "cutest.h"
#else
#  define TEST_CHECK(a) a
#  define TEST_CHECK_(a,...) a
#endif

#include "../polarssl/pk.h"
#include "../polarssl/havege.h"
#include "../polarssl/memory_buffer_alloc.h"
#include "../polarssl/error.h"

int ret;
char errorString[1000];
size_t read_file(const char* fileName, uint8_t* buffer, size_t bufferSize);
void write_file(const char* fileName, const uint8_t* buffer, size_t bufferSize);

#define TEST_CHECK_POLARSSL(cond)                                 \
  ret = cond;                                                     \
  if(ret != 0) {                                                  \
    polarssl_strerror(ret, errorString, sizeof(errorString));     \
    TEST_CHECK_(0, "expected 0 found %d (%s)", ret, errorString); \
  }

#define TEST_CHECK_ARRAY_EQ(expected, expectedLen, found, foundLen)                     \
  TEST_CHECK_(expectedLen == foundLen, "expected %d, found %d", expectedLen, foundLen); \
  for(ret = 0; ret < foundLen; ret++) {                                                 \
    TEST_CHECK_(expected[ret] == found[ret], "expected 0x%x, found 0x%x at index %d", expected[ret], found[ret], ret); \
    return;                                                                             \
  }

void test_sign() {
  pk_context pk;
  havege_state havegeState;
  uint8_t memoryBuffer[10000];
  char message[100];
  uint8_t expected[POLARSSL_MPI_MAX_SIZE];
  size_t expectedLen;
  uint8_t buffer[POLARSSL_MPI_MAX_SIZE];
  size_t olen;

  memory_buffer_alloc_init(memoryBuffer, sizeof(memoryBuffer));

  strcpy(message, "Hello World!");
  expectedLen = read_file("test_sign-expected.dat", expected, sizeof(expected));

  havege_init(&havegeState);

  pk_init(&pk);
  TEST_CHECK_POLARSSL(pk_parse_keyfile(&pk, "./test.pem", NULL));
  TEST_CHECK_POLARSSL(pk_encrypt(&pk, (uint8_t*)message, strlen(message), buffer, &olen, sizeof(buffer), havege_random, &havegeState));
  TEST_CHECK_ARRAY_EQ(expected, expectedLen, buffer, olen);
  pk_free(&pk);
  memory_buffer_alloc_free();
}

void test_verify() {
  pk_context pk;
  havege_state havegeState;
  uint8_t memoryBuffer[10000];
  char expected[100];
  size_t expectedLen;
  char found[1000];
  size_t foundLen;
  uint8_t buffer[POLARSSL_MPI_MAX_SIZE];
  size_t bufferLen;

  memory_buffer_alloc_init(memoryBuffer, sizeof(memoryBuffer));

  strcpy(expected, "Hello World!");
  expectedLen = strlen(expected);
  bufferLen = read_file("test_verify-input.dat", buffer, sizeof(buffer));

  havege_init(&havegeState);

  pk_init(&pk);
  TEST_CHECK_POLARSSL(pk_parse_public_keyfile(&pk, "./test.pub"));
  TEST_CHECK_POLARSSL(pk_verify(&pk, buffer, bufferLen, found, &foundLen, sizeof(found), havege_random, &havegeState));
  TEST_CHECK_ARRAY_EQ(expected, expectedLen, found, foundLen);
  pk_free(&pk);
  memory_buffer_alloc_free();
}

size_t read_file(const char* fileName, uint8_t* buffer, size_t bufferSize) {
  size_t len;
  FILE* f = fopen(fileName, "rb");
  if (f == NULL) {
    printf("Could not open file %s\n", fileName);
    return;
  }
  len = fread(buffer, 1, bufferSize, f);
  fclose(f);
  return len;
}

void write_file(const char* fileName, const uint8_t* buffer, size_t bufferSize) {
  FILE* f = fopen(fileName, "wb");
  fwrite(buffer, 1, bufferSize, f);
  fclose(f);
  printf("wrote file %s\n", fileName);
}

#ifdef USE_CUTEST
TEST_LIST = {
  { "sign", test_sign },
  { "verify", test_verify },
  { 0 }
};
#else
int main(int argc, char* argv[]) {
  //test_sign();
  test_verify();
  return 0;
}
#endif
