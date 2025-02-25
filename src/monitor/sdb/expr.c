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

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

enum {
  TK_NOTYPE = 256, TK_EQ,

  /* TODO: Add more token types */
  TK_INT, TK_HEX,
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"-", '-'},           // minus
  {"\\*", '*'},         // multiply
  {"/", '/'},           // divide
  {"==", TK_EQ},        // equal
  {"\\(", '('},         // parentheses left
  {"\\)", ')'},         // parentheses right
  {"[0-9]+", TK_INT},     // integer in dec
  {"0[xX][0-9a-fA-F]+", TK_HEX},  // integer in hex
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
        if (rules[i].token_type == TK_NOTYPE) {
          break;
        }

        tokens[nr_token].type = rules[i].token_type;

        switch (rules[i].token_type) {
          case TK_INT:
          case TK_HEX:
            Assert(substr_len < 32, "%s: length exceeds", substr_start);
            memcpy(tokens[nr_token].str, substr_start, substr_len);
            break;
        }
        nr_token++;
        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

/**
 * @brief check if the expression is surrounded by a matched pair of parentheses.
 */
bool check_parentheses(int p, int q, bool *legal) {
  // printf("check_parentheses: p = %d, q = %d\n", p, q);
  if (p >= q) {
    return false;
  }

  if ((tokens[p].type == '(') && (tokens[q].type == ')')) {
    int parens_pair = 1;
    bool pass = true;
    for (int i = p + 1; i < q; ++i) {
      if (tokens[i].type == '(') {
        parens_pair++;
      } else if (tokens[i].type == ')') {
        parens_pair--;
      }

      if (parens_pair < 1) {
        pass = false;
      }
      if (parens_pair < 0) {
        *legal = false;
      }
    }
    // printf("check_parentheses: res = %d\n", pass);
    return pass;
  }

  // printf("check_parentheses: res = %d\n", false);
  return false;
}

/**
 * @brief find the main operator 
 */
int find_main_op_pos(int p, int q, bool *legal) {
  // printf("find_main_op_pos: p = %d, q = %d\n", p, q);
  int main_op_pos = -1;
  int parent_pair = 0;
  for (int i = p; i <= q; ++i) {
    if (tokens[i].type == '(') {
      parent_pair++;
    } else if (tokens[i].type == ')') {
      parent_pair--;
    }
    if (parent_pair > 0) {
      continue;
    } else if (parent_pair < 0) {
      *legal = false;
      break;
    } else if (parent_pair == 0) {
      // printf("token type: %c\n", tokens[i].type);
      // printf("tokens[i].type == '+' : %d\n", (tokens[i].type == '+'));
      if ((tokens[i].type == '+') || (tokens[i].type == '-')) {
        main_op_pos = i;
      } else if ((tokens[i].type == '*') || (tokens[i].type == '/')) {
        if (main_op_pos == -1) {
          main_op_pos = i;
        } else {
          if ((tokens[main_op_pos].type == '*') || (tokens[main_op_pos].type != '/')) {
            main_op_pos = i;
          }
        }
      }
    }
  }
  // printf("main_op_pos = %d\n", main_op_pos);
  if (main_op_pos == -1) {
    *legal = false;
  }
  // printf("*legal = %d\n", *legal);
  return main_op_pos;
}

/**
 * @brief evaluate the expression stored in `tokens`.
 */
word_t eval(int p, int q, bool *legal) {
  if (p > q) {
    *legal = false;
    printf("index error: p=%d, q=%d\n", p, q);
  } else if (p == q) {
    if ((tokens[p].type == TK_INT) || (tokens[p].type == TK_HEX)) {
      char *endptr;
      int base = (tokens[p].type == TK_INT) ? 10 : 16;
      long num = strtol(tokens[p].str, &endptr, base);
      if (*endptr != '\0') {
        *legal = false;
        printf("%s is not a valid number, please try again.\n", tokens[p].str);
      }
      return num;
    } else {
      *legal = false;
      printf("%s should be a number, please try again.\n", tokens[p].str);
    }
  } else if (check_parentheses(p, q, legal)) {
    return eval(p + 1, q - 1, legal);
  } else {
    int op_pos = find_main_op_pos(p, q, legal);
    if (! (*legal)) {
      printf("failed to find main op\n");
      return 0;
    }
    switch (tokens[op_pos].type) {
      case '+': return eval(p, op_pos - 1, legal) + eval(op_pos + 1, q, legal); break;
      case '-': return eval(p, op_pos - 1, legal) - eval(op_pos + 1, q, legal); break;
      case '*': return eval(p, op_pos - 1, legal) * eval(op_pos + 1, q, legal); break;
      case '/': 
        word_t divisor = eval(op_pos + 1, q, legal);
        if (divisor == 0) {
          *legal = false;
          printf("illegal divisor: 0\n");
        } else {
          return eval(p, op_pos - 1, legal) / divisor;
        }
        break;
      default: *legal = false; break;
    }
  }
  return 0;
}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  *success = true;
  word_t res = eval(0, nr_token - 1, success);
  return res;
}
