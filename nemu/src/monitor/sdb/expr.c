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
  {"==", TK_EQ},        // equal
  {"0x[a-fA-F0-9]+", TK_HEX},   // HEX
  {"[0-9]+", TK_DEC},     // DEC
  {"$0|ra|sp|gp|tp|t[0-6]|s[0-9]|a[0-7]|s10|s11|pc", TK_REG},// REG
  {"\\+", '+'},         // plus
  {"\\-", '-'},         // minus
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

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        int j=pmatch.rm_so;
        printf("\033[31mToken:\t");
        for(;j<pmatch.rm_eo;j++){
          printf("%c",e[position+j]);
        }
        printf("\n\033[0m");

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
          case TK_EQ: tokens[nr_token].type=TK_EQ; getSubStr(substr_start, 0, substr_len,tokens[nr_token].str); nr_token++; break;
          case TK_HEX: tokens[nr_token].type=TK_HEX; getSubStr(substr_start, 0, substr_len,tokens[nr_token].str); nr_token++; break;
          case TK_DEC: tokens[nr_token].type=TK_DEC; getSubStr(substr_start, 0, substr_len,tokens[nr_token].str); nr_token++; break;
          case TK_REG: tokens[nr_token].type=TK_REG; getSubStr(substr_start, 0, substr_len,tokens[nr_token].str); nr_token++; break;
          case '+': tokens[nr_token].type='+'; getSubStr(substr_start, 0, substr_len,tokens[nr_token].str); nr_token++; break;
          case '-': tokens[nr_token].type='-'; getSubStr(substr_start, 0, substr_len,tokens[nr_token].str); nr_token++; break;
          case '*': tokens[nr_token].type='*'; getSubStr(substr_start, 0, substr_len,tokens[nr_token].str); nr_token++; break;
          case '/': tokens[nr_token].type='/'; getSubStr(substr_start, 0, substr_len,tokens[nr_token].str); nr_token++; break;
          case '(': tokens[nr_token].type='('; getSubStr(substr_start, 0, substr_len,tokens[nr_token].str); nr_token++; break;
          case ')': tokens[nr_token].type=')'; getSubStr(substr_start, 0, substr_len,tokens[nr_token].str); nr_token++; break;

          default: Log("This token is not matched: %s", substr_start); return false;
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

bool check_parentheses(int p, int q) {
  if(tokens[p].str[0]!='('||tokens[q].str[0]!=')')
    return false;
  if(tokens[p].str[0] == '(') {
    int i=p+1;
    int count=1;
    for(;i<=q;i++){
      if(!count)
        return false;
      if(tokens[i].str[0]=='(')
        count++;
      else if(tokens[i].str[0]==')')
        count--;
    }
    if(count != 0)
      return false;
  }
  return true;
};

char getMainOp(int p, int q, int* position) {
  int i=p;
  int count=0;
  char mainOp=-1;
  int mainOpPriority=-1;
  for(;i<=q;i++){
    if(tokens[i].str[0]=='(')
      count++;
    else if(tokens[i].str[0]==')')
      count--;
    else if(count==0){
      if(tokens[i].str[0]=='+'||tokens[i].str[0]=='-'){
        if(mainOpPriority<1){
          mainOpPriority=1;
          mainOp=tokens[i].str[0];
          *position = i;
        }
      }
      else if(tokens[i].str[0]=='*'||tokens[i].str[0]=='/'){
        if(mainOpPriority<2){
          mainOpPriority=2;
          mainOp=tokens[i].str[0];
          *position = i;
        }
      }
    }
  }
  return mainOp;
};

word_t eval(int p, int q) {
  if(p>q) {
    printf("1\n");
    //printf("p: %d, p_val:%s, q: %d, q_val:%s\n", p, tokens[p].str, q, tokens[q].str);
    return -1;
  } else if(p == q) {
    printf("2\n");
    printf("p: %d, p_val:%s, q: %d, q_val:%s\n", p, tokens[p].str, q, tokens[q].str);
    word_t ret;
    if(tokens[p].type == TK_HEX) {
      sscanf(tokens[p].str+2, "%08x", &ret);
      printf("str is: %s, ret is: %08x\n", tokens[p].str, ret);
      return ret;
    } else if(tokens[p].type == TK_DEC) {
      sscanf(tokens[p].str, "%ud", &ret);
      printf("str is: %s, ret is: %08d\n", tokens[p].str, ret);
      return ret;
    } else {
      printf("This token is not a valueable thing: %s\n", tokens[p].str);
      return -1;
    }
  } else if(check_parentheses(p,q) == true) {
    printf("3\n");
    //printf("p: %d, p_val:%s, q: %d, q_val:%s\n", p, tokens[p].str, q, tokens[q].str);
    return eval(p+1, q-1);
  } else {
    printf("4\n");
    //printf("p: %d, p_val:%s, q: %d, q_val:%s\n", p, tokens[p].str, q, tokens[q].str);
    int posi;
    char op_type = getMainOp(p, q, &posi);
    printf("position: %d\n", posi);
    word_t val1 = eval(p, posi-1);
    word_t val2 = eval(posi+1, q);
    printf("val1: %d, op: %c, val2: %d\n", val1, op_type, val2);

    switch (op_type)
    {
    case '+':
      return val1+val2;
    case '-':
      return val1-val2;
    case '*':
      return val1*val2;
    case '/':
      return val1/val2;
    default:
      printf("Wrong Operator\n");
      assert(0);
    }
  }
};

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  for(int i=0;i<nr_token;i++){
    printf("Token[%d]:\ttype=%d\tstr=%s\n",i,tokens[i].type,tokens[i].str);
  }

  word_t ret = eval(0, nr_token-1);
  *success = true;
  return ret;

  return 0;
}
