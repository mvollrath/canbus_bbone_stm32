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
#include <sys/time.h>

#include <linux/can.h>
#include <linux/can/raw.h>

void print_usage(char* argv0) {
  char* my_name = basename(argv0);

  printf("%s: Send a counter message to CAN Bus every second.\n\n", my_name);
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

int main(int argc, char** argv) {
  int status;
  int sfd;
  struct sockaddr_can addr;
  struct can_frame frame;
  struct ifreq ifr;
  struct sigaction sa;
  struct itimerval timer;

  // default values for opts
  char ifname[IFNAMSIZ] = "";
  canid_t id = 0x7FF;

  // parse opts
  status = parse_cmdline(argc, argv, ifname, &id);
  if (status != 0) {
    return EXIT_FAILURE;
  }

  printf("using interface %s and id %02X\n", ifname, id);

  // open can socket
  sfd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
  if (sfd < 0) {
    perror("opening socket");
    return EXIT_FAILURE;
  }

  // find interface index
  strcpy(ifr.ifr_name, ifname);
  status = ioctl(sfd, SIOCGIFINDEX, &ifr);
  if (status < 0) {
    perror("finding interface index");
    return EXIT_FAILURE;
  }

  // init can socket binding
  addr.can_family  = AF_CAN;
  addr.can_ifindex = ifr.ifr_ifindex; 

  // bind can socket
  status = bind(sfd, (struct sockaddr *)&addr, sizeof(addr));
  if (status < 0) {
    perror("binding socket");
    return EXIT_FAILURE;
  }

  // init can frame
  frame.can_id  = id;
  frame.can_dlc = 1;

  // define timer callback
  void ping(int signum) {
    static unsigned char counter = 0;
    int nbytes;

    #ifdef DEBUG
    printf("ping : %02X\n", counter);
    #endif

    frame.data[0] = counter++;
    nbytes = write(sfd, &frame, sizeof(struct can_frame));
    if (nbytes != sizeof(frame)) {
      fprintf(stderr, "ERROR: Wrote %d bytes, expected %u\n", nbytes, sizeof(frame));
      //abort();
    }
  }

  // set timer callback
  memset (&sa, 0, sizeof(sa));
  sa.sa_handler = &ping;
  status = sigaction(SIGALRM, &sa, NULL);
  if (status < 0) {
    perror("setting sigaction");
    return EXIT_FAILURE;
  }

  // init timer
  timer.it_value.tv_sec = 1;
  timer.it_value.tv_usec = 0;
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = 500000;

  // start timer
  status = setitimer (ITIMER_REAL, &timer, NULL);
  if (status < 0) {
    perror("setting timer");
    return EXIT_FAILURE;
  }

  while (1) {
    pause();
  }

  return EXIT_SUCCESS;
}
