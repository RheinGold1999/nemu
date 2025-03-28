/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"
#include <stdlib.h>

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

struct watchpoint;
typedef struct watchpoint WP;

WP* new_wp();
void free_wp(WP *wp);
void free_wp_by_id(int id);
int get_wp_id(const WP *wp);
void set_wp_expr(WP *wp, const char *e);
void set_wp_val(WP *wp, word_t val);
WP* get_wp_head();
WP* get_wp_next(const WP *wp);
void print_wp(const WP *wp);
void free_all_wp();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args) {
  uint64_t step_num = 1;

  if (args != NULL) {
    char *endptr;
    step_num = strtoul(args, &endptr, 10);

    if (*endptr != '\0') {
      printf("%s is not a valid number, please try again.\n", args);
      return 0;
    }
  }
  cpu_exec(step_num);
  return 0;
}

static int cmd_info(char *args) {
  if (strcmp(args, "r") == 0) {
    isa_reg_display();
  } else if (strcmp(args, "w") == 0) {
    WP *wp = get_wp_head();
    while (wp != NULL) {
      print_wp(wp);
      wp = get_wp_next(wp);
    }
  } else {
    printf("Unsupported args: %s, please try again.\n", args);
  }
  return 0;
}

static int cmd_x(char *args) {
  if (args != NULL) {
    char *arg = strtok(NULL, " ");
    char *endptr;
    long count = strtol(arg, &endptr, 10);
    if (*endptr != '\0') {
      printf("%s is not a valid number, please try again.\n", arg);
      return 0;
    } else if (count < 1) {
      printf("%s must be a positive number, please try again.\n", arg);
      return 0;
    }
    arg = strtok(NULL, " ");
    vaddr_t addr = strtoul(arg, &endptr, 16);
    if (*endptr != '\0') {
      printf("%s is not a valid number(hex), please try again.\n", arg);
      return 0;
    }
    uint32_t output_bytes = 0;
    extern word_t vaddr_read(vaddr_t addr, int len);
    while (output_bytes < count) {
      printf("mem[0x%08x] = 0x%08x\n", addr, vaddr_read(addr, 4));
      addr += 4;
      output_bytes += 4;
    }
  } else {
    printf("Usage: x [count (in bytes)] [address(in hex)], please try again.\n");
  }

  return 0;
}

static int cmd_p(char *args) {
  if (args != NULL) {
    bool success;
    extern word_t expr(char *e, bool *success);
    word_t res = expr(args, &success);
    if (success) {
      printf("%d\n", res);
    } else {
      printf("illegal expr: %s, please check and try again.\n", args);
    }
  } else {
    printf("Usage: p [EXP].\n");
  }
  return 0;
}

static int cmd_w(char *args) {
  if (args != NULL) {
    bool success;
    word_t val = expr(args, &success);
    if (success) {
      WP *wp = new_wp();
      if (wp != NULL) {
        set_wp_expr(wp, args);
        set_wp_val(wp, val);
      }
    } else {
      printf("invalid expressios: %s, please try again.\n", args);
    }
  } else {
    printf("Usage: w [EXP].\n");
  }
  return 0;
}

static int cmd_d(char *args) {
  if (args != NULL) {
    char *endptr;
    long id = strtol(args, &endptr, 0);
    if (*endptr == '\0') {
        free_wp_by_id(id);
    } else {
      printf("%s is invalid, use `info w` to check in-use watchpoints!\n", args);
    }
  } else {
    free_all_wp();
  }
  return 0;
}

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {

  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },

  /* TODO: Add more commands */
  { "si", "Setp program, proceeding through subroutine calls", cmd_si },
  { "info", "Generic command for showing things about the program being debugged", cmd_info },
  { "x", "Examine memory: x [count(in bytes)] [address(in hex)]", cmd_x },
  { "p", "Print value of expression EXP", cmd_p },
  { "w", "Set a watchpoint for EXPRESSION", cmd_w },
  { "d", "Delete all or some watchpoints.", cmd_d },

};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
