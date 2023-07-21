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
#include <cpu/cpu.h>
#include <memory/vaddr.h>

enum {
  TK_NOTYPE = 0, TK_EQ,

  /* TODO: Add more token types */
  TK_HEX,
  TK_DEC,
  TK_REG,
  TK_EQL,
  TK_NEQL,
  TK_AND,
  TK_DEREF,
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
  {"\\$([a-zA-Z0-9]+|\\$0)", TK_REG},// REG
  {"\\+", '+'},         // plus
  {"\\-", '-'},         // minus
  {"\\*", '*'},         // multiply or dereference
  {"\\/", '/'},         // divide
  {"\\==", TK_EQL},         // equal
  {"\\!=", TK_NEQL},         // not equal
  {"\\&&", TK_AND},         // and
  {"\\(", '('},         // left bracket
  {"\\)", ')'},         // right bracket
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};
static int isDivByZero = 0;
static int isReg = 1;

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
        //printf("\033[31mToken:\t");
        for(;j<pmatch.rm_eo;j++){
          //printf("%c",e[position+j]);
        }
        //printf("\n\033[0m");

        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        //Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            //i, rules[i].regex, position, substr_len, substr_len, substr_start);

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
          case TK_EQL: tokens[nr_token].type=TK_EQL; getSubStr(substr_start, 0, substr_len,tokens[nr_token].str); nr_token++; break;
          case TK_NEQL: tokens[nr_token].type=TK_NEQL; getSubStr(substr_start, 0, substr_len,tokens[nr_token].str); nr_token++; break;
          case TK_AND: tokens[nr_token].type=TK_AND; getSubStr(substr_start, 0, substr_len,tokens[nr_token].str); nr_token++; break;
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
      if(tokens[i].str[0]=='&') {
        if(mainOpPriority<=5){
          mainOpPriority=5;
          mainOp=TK_AND;
          *position = i;
        }
      }
      else if(tokens[i].str[0]=='='||tokens[i].str[0]=='!'){
        if(mainOpPriority<=4){
          mainOpPriority=4;
          if(tokens[i].type == TK_NEQL) {
            mainOp = TK_NEQL;
          } else {
            mainOp = TK_EQL;
          }
          *position = i;
        }
      }
      else if(tokens[i].type=='+'||tokens[i].type=='-'){
        if(mainOpPriority<=3){
          mainOpPriority=3;
          mainOp=tokens[i].str[0];
          *position = i;
        }
      }
      else if(tokens[i].type=='*'||tokens[i].type=='/'){
        if(mainOpPriority<=2){
          mainOpPriority=2;
          mainOp=tokens[i].str[0];
          *position = i;
        }
      }
      else if(tokens[i].type == TK_DEREF) {
        if(mainOpPriority<=1){
          mainOpPriority=1;
          mainOp=TK_DEREF;
          *position = i;
        }
      }
    }
  }
  return mainOp;
};

word_t hexToU32(char* str) {
  word_t ret=0;
  int i = 2;
  for(;i<=9;i++) {
    if(str[i]>='0'&&str[i]<='9')
      ret = (str[i]-'0') + ret*16;
    else if(str[i]>='a'&&str[i]<='f')
      ret = (str[i]-'a'+10) + ret*16;
    else if(str[i]>='A'&&str[i]<='F')
      ret = (str[i]-'A'+10) + ret*16;
    else
      return -1;
  }
  return ret;
}

word_t eval(int p, int q) {
  if(p>q) {
    //printf("1\n");
    return -1;
  } else if(p == q) {
    word_t ret;
    if(tokens[p].type == TK_HEX) {
      //sscanf(tokens[p].str+2, "%ux", &ret);
      ret = hexToU32(tokens[p].str);
      //printf("ret=%u\n", ret);
      return ret;
    } else if(tokens[p].type == TK_REG) {
      bool success = true;
      word_t ret = isa_reg_str2val(tokens[p].str, &success);
      if(success) {
        isReg = true;
        return ret;
      } else {
        isReg = false;
        printf("Not a valid register: %s\n", tokens[p].str);
        return -1;
      }
    } else if(tokens[p].type == TK_DEC) {
      sscanf(tokens[p].str, "%ud", &ret);
      return ret;
    } else {
      printf("This token is not a valueable thing: %s\n", tokens[p].str);
      return -1;
    }
  } else if(check_parentheses(p,q) == true) {
    return eval(p+1, q-1);
  } else {
    //printf("4\n");
    int posi;
    char op_type = getMainOp(p, q, &posi);

    //printf("posi=%d, op: %c\n", posi, op_type);
    int val1 = eval(p, posi-1);
    int val2 = eval(posi+1, q);

    //printf("val1=%u, op: %c, val2=%u, val = %u. \n", val1, op_type, val2, val1+val2);

    switch (op_type)
    {
    case TK_DEREF:
      //printf("posi=%d, op: %c\n", posi, op_type);
      int val = eval(posi+1, q);
      //printf("val=%u\n", val);
      return vaddr_read(val, 4);
    case TK_EQL:
      return val1==val2;
    case TK_NEQL:
      return val1!=val2;
    case TK_AND:
      return val1&&val2;
    case '+':
      return val1+val2;
    case '-':
      return val1-val2;
    case '*':
      return val1*val2;
    case '/':
      if(val2 == 0) {
        printf("Divide by zero\n");
        isDivByZero = 1;
        return 0;
      }
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
    if(tokens[i].type == '*' && (i==0 || (tokens[i-1].type != TK_DEC && tokens[i-1].type != TK_HEX && tokens[i-1].type != TK_REG && tokens[i-1].str[0] != ')'))) {
      tokens[i].type = TK_DEREF;
    }

    //printf("Token[%d]:  type= %d\tstr= %s\n",i,tokens[i].type,tokens[i].str);
  }

  word_t ret = eval(0, nr_token-1);

  if(isDivByZero == 1) {
    *success = false;
    isDivByZero = 0;
    //printf("isSuccess? :%d\n", *success);
    return 0;
  }

  if(isReg == 0) {
    *success = false;
    isReg = 1;
    //printf("isSuccess? :%d\n", *success);
    return 0;
  }

  isDivByZero = 0;
  *success = true;
  //printf("isSuccess? :%d\n", *success);
  return ret;

  return 0;
}
