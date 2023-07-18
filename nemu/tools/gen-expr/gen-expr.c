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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

// this should be enough
static int ntok = 0;
static int bufPtr = 0;
static int isFull = 0;
static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\\n\", result); "
"  return 0; "
"}";

uint32_t choose(uint32_t n) {
  return rand() % n;
}

void writeToBuf(char *str, int len) {
  if(bufPtr + len >= sizeof(buf)) {
    isFull = 1;
    printf("buf is full\n");
  }
  printf("str:%s\n", str);
  //memcpy(buf + bufPtr, str, len);

  int i;
  for(i=0;i<len;i++){
    buf[bufPtr++] = str[i];
  }

  printf("buf:%s\n", buf);
  //bufPtr += len;
}

int checknTok() {
  if(ntok > 32) {
    return 1;
  }
  return 0;
}

static void gen_num() {
  char temp[32];
  uint32_t num = rand() % INT32_MAX / 10 + 1;
  switch (choose(2))
  {
  case 0:
    sprintf(temp, "0x%08x\0", num);
    writeToBuf(temp, sizeof(10));
    //printf("Hex: %s\n", temp);
    break;
  default:
    sprintf(temp, "%d\0", num);
    writeToBuf(temp, sizeof(temp));
    //printf("Dec: %s\n", temp);
    break;
  }
}

static void gen(char c) {
  char temp = c;
  writeToBuf(&temp, 1);
}

static void gen_rand_op() {
  switch (choose(4))
  {
  case 0:
    gen('+');
    break;
  case 1:
    gen('-');
    break;
  case 2:
    gen('*');
    break;
  default:
    gen('/');
    break;
  }
}

static void gen_rand_expr() {
  switch (choose(3))
  {
  case 0:
    gen_num();
    break;
  case 1:
    gen('(');
    gen_rand_expr();
    gen(')');
    break;
  
  default:
    gen_rand_expr();
    gen_rand_op();
    gen_rand_expr();
    break;
  }
}

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    memset(buf, 0, sizeof(buf));
    memset(code_buf, 0, sizeof(code_buf));
    bufPtr = 0;
    isFull = 0;
    gen_rand_expr();

    if(isFull)
      continue;

    //printf("%s\n", buf);
    sprintf(code_buf, code_format, buf);
    //printf("%s\n", code_buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc /tmp/.code.c -o /tmp/.expr");
    if (ret != 0) continue;

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);


    int result;
    ret = fscanf(fp, "%d", &result);
    pclose(fp);

    printf("%u %s\n", result, buf);
  }
  return 0;
}
