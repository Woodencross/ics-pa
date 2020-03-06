#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum
{
  TK_NOTYPE = 256,
  TK_EQ,
  TK_NEQ,
  TK_AND,
  TK_OR,
  TK_NUMBER,
  TK_HEX,   //十六进制
  TK_NEG,   //负号
  TK_DEREF, //解引用
  TK_REG,

  /* TODO: Add more token types */

};

static struct rule
{
  char *regex;
  int token_type;
} rules[] = {

    /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

    {" +", TK_NOTYPE}, // spaces
    {"\\+", '+'},      // plus
    {"-", '-'},        //minus
    {"\\*", '*'},      //multiply
    {"\\/", '/'},      //divide
    {"\\(", '('},
    {"\\)", ')'},
    {"==", TK_EQ}, // equal
    {"0x[1-9A-Fa-f][0-9A-Fa-f]*", TK_HEX},
    {"0|[1-9][0-9]*", TK_NUMBER},
    {"!=", TK_NEQ},
    {"&&", TK_AND},
    {"\\|\\|", TK_OR},
    {"!", '!'},
    {"\\$(eax|ecx|edx|ebx|esp|ebp|esi|edi|eip|ax|cx|dx|bx|sp|bp|si|di|al|cl|dl|bl|ah|ch|dh|bh)", TK_REG},

};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]))

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex()
{
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i++)
  {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0)
    {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token
{
  int type;
  char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e)
{
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0')
  {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i++)
    {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0)
      {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);
        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
        if (substr_len > 31)
        { //32 or 31?
          assert(0);
        }
        if (rules[i].token_type == TK_NOTYPE)
        {
          break;
        }
        tokens[nr_token].type = rules[i].token_type;
        switch (rules[i].token_type)
        {
        case TK_NUMBER:
          strncpy(tokens[nr_token].str, substr_start, substr_len);
          *(tokens[nr_token].str + substr_len) = '\0';
          break;
        case TK_HEX:
          strncpy(tokens[nr_token].str, substr_start + 2, substr_len - 2);
          break;
        case TK_REG:
          strncpy(tokens[nr_token].str, substr_start + 1, substr_len - 1);
          break;
        default:;
        }
        printf("Success record : nr_token=%d, type=%d, str=%s\n", nr_token, tokens[nr_token].type, tokens[nr_token].str);
        nr_token++;
        break;
      }
    }

    if (i == NR_REGEX)
    {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

bool check_parentheses(int p, int q)
{
  if (p >= q)
  {
    printf("error:p>=q in check_parentheses()\n");
    return false;
  }
  if (tokens[p].type != '(' || tokens[q].type != ')')
  {
    return false;
  }
  int count = 0;
  for (int curr = p + 1; curr < q; curr++)
  {
    if (tokens[curr].type == '(')
    {
      count++;
    }
    if (tokens[curr].type == ')')
    {
      if (count > 0)
      {
        count--;
      }
      else
      {
        return false;
      }
    }
  }
  if (count == 0)
  {
    return true;
  }
  else
  {
    return false;
  }
}

int findDominantOp(int p, int q)
{
  if (p >= q)
  {
    printf("error:p>=q in findDominantOp()\n");
    return -1;
  }
  int level = 0;
  int pos[5] = {-1, -1, -1, -1, -1}; //[0]:&&,|| [1]:==,!= [2]:+,- [3]:*,/ [4]:TK_NEG, TK_DEREF, !
  for (int curr = p; curr <= q; curr++)
  {
    if (tokens[curr].type == '(')
    {
      level++;
    }
    else if (tokens[curr].type == ')')
    {
      level--;
    }
    if (level == 0)
    {
      switch (tokens[curr].type)
      {
      case TK_NEG:
      case TK_DEREF:
      case '!':
        pos[4] = curr;
        break;
      case '/':
      case '*':
        pos[3] = curr;
        break;
      case '+':
      case '-':
        pos[2] = curr;
        break;
      case TK_EQ:
      case TK_NEQ:
        pos[1] = curr;
        break;
      case TK_AND:
      case TK_OR:
        pos[0] = curr;
        break;
      default:;
      }
    }
  }
  for (int i = 0; i < 5; i++)
  {
    if (pos[i] != -1)
    {
      return pos[i];
    }
  }
  for (int i = 0; i < 5; i++)
  {
    printf("%d", pos[i]);
  }
  printf("error in findDominantOp(): p = %d, q = %d", p, q);
  assert(0);
}

int eval(int p, int q)
{
  if (p > q)
  {
    printf("bad expression in eval()\n");
    assert(0);
  }
  else if (p == q)
  {
    return atoi(tokens[p].str);
  }
  else if (check_parentheses(p, q) == true)
  {
    return eval(p + 1, q - 1);
  }
  else
  {
    int op = findDominantOp(p, q);
    if (op == -1)
    {
      assert(0);
    }
    int val_1 = eval(p, op - 1);
    int val_2 = eval(op + 1, q);
    switch (tokens[op].type)
    {
    case '+':
      return val_1 + val_2;
    case '-':
      return val_1 - val_2;
    case '*':
      return val_1 * val_2;
    case '/':
      return val_1 / val_2;
    default:
      printf("error in eval()\n");
      assert(0);
    }
  }
}

uint32_t expr(char *e, bool *success)
{
  if (!make_token(e))
  {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  //TODO();
  *success = true;
  //识别负号与解引用
  if (nr_token != 1)
  {
    if (tokens[0].type == '-')
    {
      Log("modify TK_NEG at position 0");
      tokens[0].type = TK_NEG;
    }
    if (tokens[0].type == '*')
    {
      Log("modify TK_DEREF at position 0");
      tokens[0].type = TK_DEREF;
    }
    for (int i = 1; i < nr_token; i++)
    {
      if (tokens[i].type == '-')
      {
        if (tokens[i - 1].type == '+' || tokens[i - 1].type == '-' || tokens[i - 1].type == '*' || tokens[i - 1].type == '/' || tokens[i - 1].type == '(' || tokens[i - 1].type == TK_NEG)
        {
          Log("modify TK_NEG at position %d", i);
          tokens[0].type = TK_NEG;
        }
      }
    }
  }
  return eval(0, nr_token - 1);
}
