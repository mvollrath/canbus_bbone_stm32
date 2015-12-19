#include <fcntl.h>
#include <inttypes.h>
#include <libgen.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <linux/can.h>
#include <linux/can/raw.h>

const char* LED_PATH = "/sys/class/leds/beaglebone:green:usr0/brightness";

void print_usage(char* argv0) {
  char* my_name = basename(argv0);

  printf("%s: Listen for CAN Bus pings.\n\n", my_name);
  printf("Usage:\n");
  printf("  %s -n [interface] -i [id]\n", my_name);
  printf("    -n [interface] : CAN interface. Required.\n");
  printf("    -i [id]        : CAN ID for pings. Default: 0x7FF\n");
  printf("Example:\n");
  printf("  %s -n can0\n", my_name);
}

int parse_cmdline(int argc, char** argv, char *ifname, canid_t *id) {
  int c;

  opterr = 0;

  while ((c = getopt(argc, argv, "hi:n:")) != -1)
     switch (c) {
       case 'h':
         print_usage(argv[0]);
         return 1;
       case 'i':
         *id = strtoumax(optarg, NULL, 0);
         break;
       case 'n':
         strncpy(ifname, optarg, IFNAMSIZ-1);
         break;
       case '?':
         if (optopt == 'i' || optopt == 'n')
           fprintf (stderr, "Option -%c requires an argument.\n", optopt);
         else if (isprint (optopt))
           fprintf (stderr, "Unknown option `-%c'.\n", optopt);
         else
           fprintf (stderr,
                    "Unknown option character `\\x%x'.\n",
                    optopt);
         print_usage(argv[0]);
         return 1;
       default:
         print_usage(argv[0]);
         abort();
       }

  if (ifname[0] == '\0') {
    fputs("option -n [ifname] is required.\n", stderr);
    print_usage(argv[0]);
    return 1;
  }

  return 0;
}

int set_led_brightness(int ledfd, unsigned char brightness) {
  if (brightness > 1) {
    fputs("invalid brightness value (must be 0 or 1)\n", stderr);
    return -1;
  }

  char val = brightness + 0x30;

  size_t written = write(ledfd, &val, 1);

  if (written < 0) {
    perror("setting LED brightness");
    return -1;
  }

  if (written != 1) {
    fputs("wrote incorrect size to LED\n", stderr);
    return -1;
  }

  return 0;
}

int main(int argc, char** argv) {
  int status;
  int sfd;
  int ledfd;
  struct sockaddr_can addr;
  struct can_frame frame;
  struct ifreq ifr;

  // default values for opts
  char ifname[IFNAMSIZ] = "";
  canid_t id = 0x7FF;

  // parse opts
  status = parse_cmdline(argc, argv, ifname, &id);
  if (status != 0) {
    exit(EXIT_FAILURE);
  }

  printf("using interface %s and id %02X\n", ifname, id);

  // open can socket
  sfd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
  if (sfd < 0) {
    perror("opening socket");
    exit(EXIT_FAILURE);
  }

  // find interface index
  strcpy(ifr.ifr_name, ifname);
  status = ioctl(sfd, SIOCGIFINDEX, &ifr);
  if (status < 0) {
    perror("finding interface index");
    exit(EXIT_FAILURE);
  }

  // init can socket binding
  addr.can_family  = AF_CAN;
  addr.can_ifindex = ifr.ifr_ifindex; 

  // bind can socket
  status = bind(sfd, (struct sockaddr *)&addr, sizeof(addr));
  if (status < 0) {
    perror("binding socket");
    exit(EXIT_FAILURE);
  }

  // open LED
  ledfd = open(LED_PATH, O_WRONLY);
  if (ledfd < 0) {
    perror("opening LED");
    exit(EXIT_FAILURE);
  }

  // define frame callback
  int read_frame() {
    unsigned char counter;
    int nbytes = read(sfd, &frame, sizeof(struct can_frame));

    if (nbytes < 0) {
      perror("can frame raw socket read");
      return -1;
    }

    if (nbytes != sizeof(struct can_frame)) {
      fputs("incomplete can frame\n", stderr);
      return -1;
    }

    if (frame.can_dlc != 1) {
      fputs("unexpected ping frame size\n", stderr);
      return -1;
    }

    if (frame.can_id != id) {
      return 0;
    }

    counter = frame.data[0];

    #ifdef DEBUG
    printf("pong : %02X : %02X\n", frame.can_id, counter);
    #endif

    status = set_led_brightness(ledfd, counter % 2);
    if (status != 0) {
      return -1;
    }

    return 0;
  }

  // init LED brightness
  status = set_led_brightness(ledfd, 0);
  if (status != 0) {
    fputs("failed to init LED state\n", stderr);
    exit(EXIT_FAILURE);
  }

  // define exit handler(s)
  void cleanup() {
    #ifdef DEBUG
    printf("cleaning up\n");
    #endif

    status = set_led_brightness(ledfd, 0);
    if (status != 0) {
      fputs("failed to reset LED at exit\n", stderr);
    }
  }
  void cleanup_sig(int signum) {
    exit(EXIT_SUCCESS);
  }

  // attach exit handler(s)
  if (signal(SIGINT, cleanup_sig) == SIG_ERR) {
    fputs("failed to set SIGINT handler\n", stderr);
    exit(EXIT_FAILURE);
  }
  if (signal(SIGTERM, cleanup_sig) == SIG_ERR) {
    fputs("failed to set SIGTERM handler\n", stderr);
    exit(EXIT_FAILURE);
  }
  atexit(cleanup);

  // main loop
  while (1) {
    status = read_frame();
    if (status != 0) {
      fputs("error during read, exiting\n", stderr);
      exit(EXIT_FAILURE);
    }
  }

  return EXIT_SUCCESS;
}
