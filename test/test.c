#include "test.h"

#include <isa.h>

void test() {
  test_expr();
}

void test_expr() {
  bool success = false;
  extern word_t expr(char *e, bool *success);

  char *expr_str = "( ( 137 ) - 177 * 144 + 215 ) - ( 25 ) - ( 0 * 156 * 43 / 139 - 118 + 16 * 149 )";
  uint32_t res = expr(expr_str, &success);
  uint32_t res_ref = ( ( 137 ) - 177 * 144 + 215 ) - ( 25 ) - ( 0 * 156 * 43 / 139 - 118 + 16 * 149 );
  if (res != res_ref) {
    printf("Failed: res = %u, res_ref = %u", res, res_ref);
  } else {
    printf("Passed: %s", expr_str);
  }
}
