#include <errno.h>
#include <fcntl.h>
#include <linux/ioctl.h>
#include <linux/types.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define TDX_CMD_GET_QUOTE _IOWR('T', 2, struct tdx_quote_req)
#define TDX_CMD_GET_REPORT0 _IOWR('T', 1, struct tdx_report_req)

static const unsigned HEADER_SIZE = 4;
static const size_t REQ_BUF_SIZE = 4 * 4 * 1024;  // 4 pages

#define TDX_REPORT_DATA_SIZE 64

#define TDX_REPORT_SIZE 1024

struct tdx_report_req {
  __u8 reportdata[TDX_REPORT_DATA_SIZE];
  __u8 tdreport[TDX_REPORT_SIZE];
};

struct tdx_quote_req {
  __u64 buf;
  __u64 len;
};

struct QuoteRequest {
  // Quote format version, filled by TD.
  // Format version: must be 1 in little endian.
  uint64_t version;
  uint64_t status;
  uint32_t in_len;
  uint32_t out_len;
};

int main() {
  int devfd = open("/dev/tdx_guest", O_RDWR | O_SYNC);
  if (devfd < 0) {
    // Old kernel drivers may expose the TDX attestation interface at
    // `/dev/tdx-attest`.
    devfd = open("/dev/tdx-attest", O_RDWR | O_SYNC);
    if (devfd < 0) {
      printf("Cannot open the tdx guest device\n");
      return 1;
    }
  }
  printf("Open device successfully\n");
  struct tdx_report_req req;

  if (-1 == ioctl(devfd, TDX_CMD_GET_REPORT0, &req)) {
    printf("Cannot get the tdx report. error number: %d\n", errno);
    close(devfd);
    return 1;
  }

  printf("Get TD Report successfully.\n");

  uint8_t buf[REQ_BUF_SIZE];
  struct tdx_quote_req arg;
  arg.buf = (uint64_t)(&buf);
  arg.len = REQ_BUF_SIZE;
  struct QuoteRequest* hdr = (struct QuoteRequest*)&buf;
  hdr->version = 1;
  hdr->status = 0;
  hdr->in_len = 1024;
  hdr->out_len = 0;
  memcpy(buf + sizeof(*hdr), req.tdreport, sizeof(req.tdreport));

  int ioctl_ret = ioctl(devfd, TDX_CMD_GET_QUOTE, &arg);
  if (ioctl_ret == -1) {
    printf("ioctl failed. cannot get the tdx quote. error number: %d\n", errno);
    close(devfd);
    return 1;
  }

  printf("Get TD Quote successfully.\nQuoteRequest:\n");
  for (int i = 0; i < sizeof(struct QuoteRequest); ++i) {
    printf("%02x ", buf[i]);
  }
  printf("\nFirst 128 bytes of TD quote buf:\n");
  for (int i = sizeof(struct QuoteRequest); i < 128; ++i) {
    printf("%02x ", buf[i]);
  }
  printf("\n");
  close(devfd);

  return 0;
}

