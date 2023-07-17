/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
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
  TK_NOTYPE = 0, TK_EQ,

  /* TODO: Add more token types */
  TK_HEX,
  TK_DEC,
  TK_REG,
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
  {"\\-", '-'},         // minus
  {"==", TK_EQ},        // equal
  {"0x[a-fA-F0-9]+", TK_HEX},   // HEX
  {"\\d+", TK_DEC},     // DEC
  {"\\$[a-z]+", TK_REG},// REG
  {"\\*", '*'},         // multiply
  {"\\/", '/'},         // divide
  {"\\(", '('},         // left bracket
  {"\\)", ')'},         // right bracket

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

void getSubStr(char *e, int start, int end, char *subStr)
{
  int i=start;
  for(;i<end;i++){
    subStr[i+start]=e[i];
  }
  subStr[i+start]='\0';
}

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  printf("1\n");
  printf("%s\n", e);
  printf("%d\n", TK_EQ);

  while (e[position] != '\0') {
    printf("2\n");

    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {

        printf("3\n");
        int j=pmatch.rm_so;
        for(;j<pmatch.rm_eo;j++){
          printf("%c",e[position+j]);
        }
        printf("\n");

        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
          case TK_NOTYPE: break;
          case TK_EQ: tokens[nr_token].type=TK_EQ; getSubStr(e+position, pmatch.rm_so, pmatch.rm_eo,tokens[nr_token].str); nr_token++; break;
          case TK_HEX: tokens[nr_token].type=TK_HEX; getSubStr(e+position, pmatch.rm_so, pmatch.rm_eo,tokens[nr_token].str); nr_token++; break;
          case TK_DEC: tokens[nr_token].type=TK_DEC; getSubStr(e+position, pmatch.rm_so, pmatch.rm_eo,tokens[nr_token].str); nr_token++; break;
          case TK_REG: tokens[nr_token].type=TK_REG; getSubStr(e+position, pmatch.rm_so, pmatch.rm_eo,tokens[nr_token].str); nr_token++; break;
          case '+': tokens[nr_token].type='+'; getSubStr(e+position, pmatch.rm_so, pmatch.rm_eo,tokens[nr_token].str); nr_token++; break;
          case '-': tokens[nr_token].type='-'; getSubStr(e+position, pmatch.rm_so, pmatch.rm_eo,tokens[nr_token].str); nr_token++; break;
          case '*': tokens[nr_token].type='*'; getSubStr(e+position, pmatch.rm_so, pmatch.rm_eo,tokens[nr_token].str); nr_token++; break;
          case '/': tokens[nr_token].type='/'; getSubStr(e+position, pmatch.rm_so, pmatch.rm_eo,tokens[nr_token].str); nr_token++; break;
          case '(': tokens[nr_token].type='('; getSubStr(e+position, pmatch.rm_so, pmatch.rm_eo,tokens[nr_token].str); nr_token++; break;
          case ')': tokens[nr_token].type=')'; getSubStr(e+position, pmatch.rm_so, pmatch.rm_eo,tokens[nr_token].str); nr_token++; break;


          default: TODO();
        }

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


word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  TODO();

  return 0;
}
