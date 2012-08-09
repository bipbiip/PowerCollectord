#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <syslog.h>
#include <regex.h>
#include <signal.h>

#define BAUDRATE B1200
#define MODEMDEVICE "/dev/ttyUSB0"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define DEBUG 0

volatile int STOP=FALSE;

main()
{
  int fd, c, res, regex_err, regex_match;
  struct termios oldtio, newtio;
  char buf[255], debut[1],releve[46];
  char *PTEC,*HCHC,*HCHP,*IINST,*PAPP;
  time_t now;
  regex_t preg;
  const char *str_regex = "[0-9]{10},[0-9]{9},[0-9]{9},[CP],[0-9]{3},[0-9]{5}";
  regex_err = regcomp (&preg, str_regex, REG_NOSUB | REG_EXTENDED);

  setlogmask (LOG_UPTO (LOG_INFO));
  openlog ("", LOG_NDELAY, LOG_LOCAL1);


  fd = open(MODEMDEVICE, O_RDWR | O_NOCTTY );
  if (fd <0) {perror(MODEMDEVICE); exit(.1); }

  tcgetattr(fd,&oldtio); /* save current port settings */

  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = BAUDRATE | PARENB | CS7 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;

  /* set input mode (non.canonical, no echo,...) */
  newtio.c_lflag = 0;

  newtio.c_cc[VTIME] = 0; /* inter.character timer unused */
  newtio.c_cc[VMIN] = 170; /* blocking read until 5 chars received */

  tcflush(fd, TCIFLUSH);
  tcsetattr(fd,TCSANOW,&newtio);

  res = read(fd,(char *)debut,1);
  while (debut[0] != '\x02') {
    res = read(fd,debut,1);
  }

  while (STOP==FALSE) {
    if (debut[0] == '\x02') {
      time(&now);
      res = read(fd,buf,255);
      buf[res]=0;

      HCHC  = strndup(buf+56,9);
      HCHP  = strndup(buf+74,9);
      PTEC  = strndup(buf+93,1);
      IINST = strndup(buf+106,3);
      PAPP  = strndup(buf+130,5);
      snprintf(releve,45,"%ld,%s,%s,%s,%s,%s",now,HCHC,HCHP,PTEC,IINST,PAPP);
      free(HCHC);
      free(HCHP);
      free(PTEC);
      free(IINST);
      free(PAPP);
      regex_match = regexec (&preg,releve,0,NULL,0);
      /* NOTA: regexc returns 0 if string matches */
      if (regex_match == 0) {
        syslog(LOG_INFO,"%s",releve);
      }
      else {
        while (debut[0] != '\x02') {
            res = read(fd,debut,1);
        }
      }
    }
    else {
      while (debut[0] != '\x02') {
        res = read(fd,debut,1);
      }
    }
  }
  tcsetattr(fd,TCSANOW,&oldtio);
}

