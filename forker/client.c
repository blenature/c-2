#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>


#include "get_line.h"

#include "soc.h"
#include "forker_messages.h"


/* To is:   "bla\0bla\0...bla\0\0 */
/* becomes: "bla\0bla\0...bla\0str\0\0 */
void cat_str (char *to, char *str) {
  char p;

  p = ' ';
  while ( (*to != '\0') || (p != '\0') ) {
    p = *to;
    to++;
  }
  strcpy (to, str);
  while (*to != '\0') to++;
  *(to+1) = '\0';
} 


int main(int argc, char *argv[]) {
 
  command_number number;
  soc_token soc = NULL;
  char buff[500];
  request_message_t request;
  boolean something;
  report_message_t report;
  int   report_len;
  int i, n;

  if (argc != 2) {
    fprintf(stderr, "Error. One arg (port_name) expected.\n");
    exit(1);
  }

  /* Socket stuff */
  if (soc_open(&soc) == BS_ERROR) {
    perror("soc_open");
    exit(1);
  }
  if (soc_link_dynamic(soc) == BS_ERROR) {
    perror("soc_link_dynamic");
    exit(1);
  }
  if (soc_set_blocking(soc, FALSE)  == BS_ERROR) {
    perror("soc_set_dest_service");
    exit(1);
  }

  number = 0;
  for (;;) {
    printf ("\n");
    /* Dest is affected by message reception */
    if (soc_set_dest_service(soc, "localhost", FALSE, argv[1]) == BS_ERROR) {
      perror("soc_set_dest_service");
      exit(1);
    }

    printf ("Start Kill Read (s k r) ? ");
    i = get_line (NULL, buff, sizeof(buff));

    if (strcmp(buff, "s") == 0) {
      /* Start */
      memset(request.start_req.command_text, 0,
             sizeof(request.start_req.command_text));
      memset(request.start_req.environ_variables, 0,
             sizeof(request.start_req.environ_variables));
      request.kind = start_command;
      printf ("Number: %d\n", number);
      request.start_req.number = number;
      number += 2;
      printf ("Command ? ");
      i = get_line (NULL, request.start_req.command_text, 
                  sizeof(request.start_req.command_text));
      for (;;) {
        printf ("Argument (Empty to end) ? ");
        i = get_line (NULL, buff, sizeof(buff));
        if (i == 0) break;
        cat_str (request.start_req.command_text, buff);
      }
      for (n = 1;;n++) {
        printf ("Environ (Empty to end) ? ");
        i = get_line (NULL, buff, sizeof(buff));
        if (i == 0) break;
        if (n == 1) {
          strcpy (request.start_req.environ_variables, buff);
        } else {
          cat_str (request.start_req.environ_variables, buff);
        }
      }

      printf ("Current dir ? ");
      i = get_line (NULL, request.start_req.currend_dir,
                    sizeof(request.start_req.currend_dir));

      printf ("Output flow ? ");
      i = get_line (NULL, request.start_req.output_flow,
                    sizeof(request.start_req.output_flow));
      for (;;) {
        printf ("Append (t or [f]) ? ");
        i = get_line (NULL, buff, sizeof(buff));
        if (strcmp(buff, "t") == 0) {
          request.start_req.append_output = 1;
          break;
        } else if ( (i == 0) || (strcmp(buff, "f") == 0) ) {
          request.start_req.append_output = 0;
          break;
        }
      }

      printf ("Error flow ? ");
      i = get_line (NULL, request.start_req.error_flow,
                    sizeof(request.start_req.error_flow));
      for (;;) {
        printf ("Append (t or [f]) ? ");
        i = get_line (NULL, buff, sizeof(buff));
        if (strcmp(buff, "t") == 0) {
          request.start_req.append_error = 1;
          break;
        } else if ( (i == 0) || (strcmp(buff, "f") == 0) ) {
          request.start_req.append_error = 0;
          break;
        }
      }

      if (soc_send(soc, (char*)&request, sizeof(request)) == BS_ERROR) {
        perror("soc_send");
      }

    } else if (strcmp(buff, "k") == 0) {
      /* Kill */
      request.kind = kill_command;
      for (;;) {
        printf ("Number ? ");
        i = get_line (NULL, buff, sizeof(buff));
        i = atoi(buff);
        if (i >= 0) break;
      }
      request.start_req.number = i;
      for (;;) {
        printf ("Signal ? ");
        i = get_line (NULL, buff, sizeof(buff));
        i = atoi(buff);
        if (i >= 0) break;
      }
      request.kill_req.signal_number = i;

      if (soc_send(soc, (char*)&request, sizeof(request)) == BS_ERROR) {
        perror("soc_send");
      }
      

    } else if (strcmp(buff, "r") == 0) {
      /* Read report */
      report_len = sizeof(report);
      if (soc_receive(soc, &something, (char*)&report, &report_len) == BS_ERROR) {
        perror("soc_receive");
      } else if (something) {
        printf ("Report: command %d code %d ", report.number, report.exit_code);
        if (WIFEXITED(report.number)) {
          printf ("exit normally code %d\n", WEXITSTATUS(report.exit_code));
        } else if (WIFSIGNALED(report.number)) {
          printf ("exit on signal %d\n", WTERMSIG(report.exit_code));
        } else {
          printf ("stopped on signal %d\n", WSTOPSIG(report.exit_code));
        }
      } else {
        printf ("No report\n");
      }
    }
  }
}

