/* C code produced by gperf version 3.0.3 */
/* Command-line: /usr/bin/gperf -o -i 1 -C -k '1-3,$' -L C -H keyword_hash -N check_identifier -tT ./verilog_keyword.gperf  */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gnu-gperf@gnu.org>."
#endif

#line 1 "./verilog_keyword.gperf"


#include <string.h>
#include "keyword_tokens.h"

struct verilog_keyword { const char *name; int token; };


#define TOTAL_KEYWORDS 100
#define MIN_WORD_LENGTH 2
#define MAX_WORD_LENGTH 12
#define MIN_HASH_VALUE 7
#define MAX_HASH_VALUE 214
/* maximum key range = 208, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
keyword_hash (str, len)
     register const char *str;
     register unsigned int len;
{
  static const unsigned char asso_values[] =
    {
      215, 215, 215, 215, 215, 215, 215, 215, 215, 215,
      215, 215, 215, 215, 215, 215, 215, 215, 215, 215,
      215, 215, 215, 215, 215, 215, 215, 215, 215, 215,
      215, 215, 215, 215, 215, 215, 215, 215, 215, 215,
      215, 215, 215, 215, 215, 215, 215, 215,  71,  66,
      215, 215, 215, 215, 215, 215, 215, 215, 215, 215,
      215, 215, 215, 215, 215, 215, 215, 215, 215, 215,
      215, 215, 215, 215, 215, 215, 215, 215, 215, 215,
      215, 215, 215, 215, 215, 215, 215, 215, 215, 215,
      215, 215, 215, 215, 215, 215, 215,  56,  21,  61,
        1,   1,  31,  46,  11,   6,  76,  76,  16,   1,
        1,  21,  31, 215,   1,  16,   6,  36,  31,  81,
        6,  16,   1, 215, 215, 215, 215, 215, 215, 215,
      215, 215, 215, 215, 215, 215, 215, 215, 215, 215,
      215, 215, 215, 215, 215, 215, 215, 215, 215, 215,
      215, 215, 215, 215, 215, 215, 215, 215, 215, 215,
      215, 215, 215, 215, 215, 215, 215, 215, 215, 215,
      215, 215, 215, 215, 215, 215, 215, 215, 215, 215,
      215, 215, 215, 215, 215, 215, 215, 215, 215, 215,
      215, 215, 215, 215, 215, 215, 215, 215, 215, 215,
      215, 215, 215, 215, 215, 215, 215, 215, 215, 215,
      215, 215, 215, 215, 215, 215, 215, 215, 215, 215,
      215, 215, 215, 215, 215, 215, 215, 215, 215, 215,
      215, 215, 215, 215, 215, 215, 215, 215, 215, 215,
      215, 215, 215, 215, 215, 215, 215, 215, 215, 215,
      215, 215, 215, 215, 215, 215
    };
  register int hval = len;

  switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)str[2]];
      /*FALLTHROUGH*/
      case 2:
        hval += asso_values[(unsigned char)str[1]];
      /*FALLTHROUGH*/
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval + asso_values[(unsigned char)str[len - 1]];
}

#ifdef __GNUC__
__inline
#ifdef __GNUC_STDC_INLINE__
__attribute__ ((__gnu_inline__))
#endif
#endif
const struct verilog_keyword *
check_identifier (str, len)
     register const char *str;
     register unsigned int len;
{
  static const struct verilog_keyword wordlist[] =
    {
      {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 28 "./verilog_keyword.gperf"
      {"end", V_END},
      {""}, {""},
#line 52 "./verilog_keyword.gperf"
      {"medium", V_MEDIUM},
#line 29 "./verilog_keyword.gperf"
      {"endcase", V_ENDCASE},
#line 34 "./verilog_keyword.gperf"
      {"endtable", V_ENDTABLE},
#line 31 "./verilog_keyword.gperf"
      {"endmodule", V_ENDMODULE},
#line 78 "./verilog_keyword.gperf"
      {"rtran", V_RTRAN},
#line 30 "./verilog_keyword.gperf"
      {"endfunction", V_ENDFUNCTION},
#line 32 "./verilog_keyword.gperf"
      {"endprimitive", V_ENDPRIMITIVE},
      {""},
#line 91 "./verilog_keyword.gperf"
      {"time", V_TIME},
#line 99 "./verilog_keyword.gperf"
      {"trior", V_TRIOR},
#line 98 "./verilog_keyword.gperf"
      {"triand", V_TRIAND},
#line 48 "./verilog_keyword.gperf"
      {"integer", V_INTEGER},
#line 95 "./verilog_keyword.gperf"
      {"tri", V_TRI},
      {""},
#line 76 "./verilog_keyword.gperf"
      {"rnmos", V_RNMOS},
#line 61 "./verilog_keyword.gperf"
      {"or", V_ORLIT},
#line 74 "./verilog_keyword.gperf"
      {"release", V_RELEASE},
#line 57 "./verilog_keyword.gperf"
      {"nor", V_NORLIT},
      {""},
#line 33 "./verilog_keyword.gperf"
      {"endspecify", V_ENDSPECIFY},
#line 53 "./verilog_keyword.gperf"
      {"module", V_MODULE},
#line 25 "./verilog_keyword.gperf"
      {"disable", V_DISABLE},
#line 110 "./verilog_keyword.gperf"
      {"xor", V_XORLIT},
#line 109 "./verilog_keyword.gperf"
      {"xnor", V_XNORLIT},
      {""}, {""},
#line 45 "./verilog_keyword.gperf"
      {"initial", V_INITIAL},
#line 58 "./verilog_keyword.gperf"
      {"not", V_NOTLIT},
#line 27 "./verilog_keyword.gperf"
      {"else", V_ELSE},
#line 46 "./verilog_keyword.gperf"
      {"inout", V_INOUT},
      {""}, {""},
#line 24 "./verilog_keyword.gperf"
      {"defparam", V_DEFPARAM},
#line 56 "./verilog_keyword.gperf"
      {"nmos", V_NMOS},
#line 36 "./verilog_keyword.gperf"
      {"event", V_EVENT},
#line 75 "./verilog_keyword.gperf"
      {"repeat", V_REPEAT},
#line 23 "./verilog_keyword.gperf"
      {"default", V_DEFAULT},
      {""},
#line 66 "./verilog_keyword.gperf"
      {"primitive", V_PRIMITIVE},
#line 47 "./verilog_keyword.gperf"
      {"input", V_INPUT},
      {""}, {""}, {""},
#line 26 "./verilog_keyword.gperf"
      {"edge", V_EDGE},
#line 77 "./verilog_keyword.gperf"
      {"rpmos", V_RPMOS},
      {""},
#line 55 "./verilog_keyword.gperf"
      {"negedge", V_NEGEDGE},
#line 37 "./verilog_keyword.gperf"
      {"for", V_FOR},
#line 84 "./verilog_keyword.gperf"
      {"specparam", V_SPECPARAM},
#line 38 "./verilog_keyword.gperf"
      {"force", V_FORCE},
      {""},
#line 39 "./verilog_keyword.gperf"
      {"forever", V_FOREVER},
#line 12 "./verilog_keyword.gperf"
      {"and", V_ANDLIT},
#line 54 "./verilog_keyword.gperf"
      {"nand", V_NANDLIT},
      {""},
#line 100 "./verilog_keyword.gperf"
      {"trireg", V_TRIREG},
      {""},
#line 22 "./verilog_keyword.gperf"
      {"deassign", V_DEASSIGN},
#line 92 "./verilog_keyword.gperf"
      {"tran", V_TRAN},
      {""},
#line 44 "./verilog_keyword.gperf"
      {"if", V_IF},
#line 83 "./verilog_keyword.gperf"
      {"specify", V_SPECIFY},
      {""},
#line 64 "./verilog_keyword.gperf"
      {"pmos", V_PMOS},
#line 14 "./verilog_keyword.gperf"
      {"begin", V_BEGIN},
#line 62 "./verilog_keyword.gperf"
      {"output", V_OUTPUT},
#line 65 "./verilog_keyword.gperf"
      {"posedge", V_POSEDGE},
#line 41 "./verilog_keyword.gperf"
      {"function", V_FUNCTION},
#line 72 "./verilog_keyword.gperf"
      {"real", V_REAL},
#line 50 "./verilog_keyword.gperf"
      {"large", V_LARGE},
      {""}, {""},
#line 80 "./verilog_keyword.gperf"
      {"rtranif1", V_RTRANIF1},
#line 97 "./verilog_keyword.gperf"
      {"tri1", V_TRI1},
#line 71 "./verilog_keyword.gperf"
      {"rcmos", V_RCMOS},
      {""},
#line 35 "./verilog_keyword.gperf"
      {"endtask", V_ENDTASK},
#line 79 "./verilog_keyword.gperf"
      {"rtranif0", V_RTRANIF0},
#line 96 "./verilog_keyword.gperf"
      {"tri0", V_TRI0},
#line 89 "./verilog_keyword.gperf"
      {"table", V_TABLE},
      {""}, {""},
#line 69 "./verilog_keyword.gperf"
      {"pulldown", V_PULLDOWN},
#line 107 "./verilog_keyword.gperf"
      {"wire", V_WIRE},
#line 82 "./verilog_keyword.gperf"
      {"small", V_SMALL},
#line 13 "./verilog_keyword.gperf"
      {"assign", V_ASSIGN},
#line 86 "./verilog_keyword.gperf"
      {"strong1", V_STRONG1},
#line 73 "./verilog_keyword.gperf"
      {"reg", V_REG},
#line 63 "./verilog_keyword.gperf"
      {"parameter", V_PARAMETER},
      {""},
#line 60 "./verilog_keyword.gperf"
      {"notif1", V_NOTIF1},
#line 85 "./verilog_keyword.gperf"
      {"strong0", V_STRONG0},
#line 101 "./verilog_keyword.gperf"
      {"vectored", V_VECTORED},
#line 21 "./verilog_keyword.gperf"
      {"cmos", V_CMOS},
#line 106 "./verilog_keyword.gperf"
      {"while", V_WHILE},
#line 59 "./verilog_keyword.gperf"
      {"notif0", V_NOTIF0},
      {""},
#line 108 "./verilog_keyword.gperf"
      {"wor", V_WOR},
#line 49 "./verilog_keyword.gperf"
      {"join", V_JOIN},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""},
#line 70 "./verilog_keyword.gperf"
      {"pullup", V_PULLUP},
      {""},
#line 15 "./verilog_keyword.gperf"
      {"buf", V_BUF},
      {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 51 "./verilog_keyword.gperf"
      {"macromodule", V_MACROMODULE},
      {""}, {""},
#line 40 "./verilog_keyword.gperf"
      {"fork", V_FORK},
      {""},
#line 43 "./verilog_keyword.gperf"
      {"highz1", V_HIGHZ1},
#line 94 "./verilog_keyword.gperf"
      {"tranif1", V_TRANIF1},
      {""},
#line 18 "./verilog_keyword.gperf"
      {"case", V_CASE},
#line 20 "./verilog_keyword.gperf"
      {"casez", V_CASEZ},
#line 42 "./verilog_keyword.gperf"
      {"highz0", V_HIGHZ0},
#line 93 "./verilog_keyword.gperf"
      {"tranif0", V_TRANIF0},
#line 81 "./verilog_keyword.gperf"
      {"scalared", V_SCALARED},
#line 103 "./verilog_keyword.gperf"
      {"wand", V_WAND},
#line 19 "./verilog_keyword.gperf"
      {"casex", V_CASEX},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 102 "./verilog_keyword.gperf"
      {"wait", V_WAIT},
#line 68 "./verilog_keyword.gperf"
      {"pull1", V_PULL1},
      {""},
#line 88 "./verilog_keyword.gperf"
      {"supply1", V_SUPPLY1},
      {""},
#line 90 "./verilog_keyword.gperf"
      {"task", V_TASK},
#line 67 "./verilog_keyword.gperf"
      {"pull0", V_PULL0},
#line 17 "./verilog_keyword.gperf"
      {"bufif1", V_BUFIF1},
#line 87 "./verilog_keyword.gperf"
      {"supply0", V_SUPPLY0},
      {""}, {""}, {""},
#line 16 "./verilog_keyword.gperf"
      {"bufif0", V_BUFIF0},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
#line 11 "./verilog_keyword.gperf"
      {"always", V_ALWAYS},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""},
#line 105 "./verilog_keyword.gperf"
      {"weak1", V_WEAK1},
      {""}, {""}, {""}, {""},
#line 104 "./verilog_keyword.gperf"
      {"weak0", V_WEAK0}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = keyword_hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register const char *s = wordlist[key].name;

          if (*str == *s && !strcmp (str + 1, s + 1))
            return &wordlist[key];
        }
    }
  return 0;
}
#line 111 "./verilog_keyword.gperf"


int verilog_keyword_code(const char *s, unsigned int len)
{
const struct verilog_keyword *rc = check_identifier(s, len);
return(rc ? rc->token : V_IDENTIFIER);
}
