#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sql.h"

static int yyparse(void);
static int yylex(void);
static char *quotes(int);
static struct select *select_init(void);
static struct sql_stmt s;	/* result of parsing */
typedef union  {
  double val;
  sconst num;
  const char *name;
  struct select *qry;
  sql exp;
} YYSTYPE;
# define IDENT 257
# define STRING 258
# define CONST 259
# define IS 260
# define NUL 261
# define SELECT 262
# define ORDER 263
# define BY 264
# define WHERE 265
# define FROM 266
# define DISTINCT 267
# define GROUP 268
# define ERROR 269
# define DESC 270
# define ASC 271
# define OUTER 272
# define ALL 273
# define ANY 274
# define CONNECT 275
# define INSERT 276
# define VALUES 277
# define INTO 278
# define AS 279
# define UPDATE 280
# define SET 281
# define DELETE 282
# define ALTER 283
# define MODIFY 284
# define ADD 285
# define UNIQUE 286
# define INDEX 287
# define ON 288
# define DROP 289
# define RENAME 290
# define TO 291
# define PRIOR 292
# define TABLE 293
# define VIEW 294
# define START 295
# define WITH 296
# define CLUSTER 297
# define UNION 298
# define INTERSECT 299
# define MINUS 300
# define CREATE 301
# define HAVING 302
# define USE 303
# define JOIN 304
# define CROSS 305
# define CASE 306
# define WHEN 307
# define THEN 308
# define ELSE 309
# define END 310
# define SUBSTRING 311
# define FOR 312
# define EXISTS 313
# define OR 314
# define AND 315
# define NOT 316
# define NE 317
# define LE 318
# define GE 319
# define BETWEEN 320
# define IN 321
# define LIKE 322
# define CONCAT 323
#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
extern int yychar;
extern int yyerrflag;
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif
YYSTYPE yylval, yyval;
typedef int yytabelem;
#include <stdio.h>
# define YYERRCODE 256
yytabelem yyexca[] ={
	-1, 1,
	0, -1,
	-2, 0,
-1, 11,
	304, 54,
	305, 54,
	-2, 57,
-1, 19,
	257, 75,
	-2, 28,
-1, 25,
	304, 48,
	305, 48,
	-2, 103,
-1, 100,
	257, 75,
	-2, 125,
-1, 129,
	41, 99,
	44, 99,
	-2, 73,
-1, 177,
	44, 97,
	-2, 73,
-1, 203,
	41, 103,
	44, 103,
	-2, 48,
	};
# define YYNPROD 132
# define YYLAST 712
yytabelem yyact[]={

   119,    19,    58,   103,    59,    61,   226,    62,   291,    60,
   225,   227,   228,    59,    59,    61,    53,    62,    60,    60,
   276,   229,   169,   168,   170,   259,   260,   261,    75,   253,
    80,    19,   169,   168,   170,   272,    59,    61,   166,    62,
    99,    60,    59,    61,    88,    62,   245,    60,    19,    59,
    61,   235,    62,    37,    60,   231,   104,   167,   166,   108,
   109,   110,   111,   112,   299,    59,    61,    45,    62,   186,
    60,   187,    59,    61,   129,    62,   230,    60,   189,    53,
   137,   178,    43,    19,   167,   166,    29,   142,    59,    61,
    96,    62,   120,    60,   273,    59,    61,    29,    62,    54,
    60,   167,   166,    59,    61,   159,    62,   161,    60,    59,
    61,   131,    62,   132,    60,    59,    61,    63,    62,    91,
    60,   177,    78,    59,    61,    70,    62,    29,    60,    59,
    61,   211,    62,   185,    60,   188,   103,    59,    61,   156,
    62,    56,    60,    59,    61,   194,    62,    69,    60,    59,
    61,   138,    62,   207,    60,    48,    49,    48,    49,    71,
   279,    73,    70,    33,    36,    68,    91,    71,   205,   219,
   220,   221,   222,   223,   224,   106,    86,   300,    34,   177,
    15,   144,   280,   129,    69,    29,   264,   237,   238,    15,
   240,   307,   241,   242,    29,   293,    78,   142,    84,   286,
   263,    29,    40,   216,    33,    31,   160,    20,    33,   250,
    73,    85,    42,   151,   254,   213,    71,   126,    78,    66,
    84,   211,   175,    29,   124,   107,   258,    94,   201,    64,
    78,    55,   175,    44,   208,    29,   267,   146,   210,    93,
   270,   147,   141,   290,    24,    89,    93,   196,    81,   147,
   277,    47,   200,    89,    95,    71,    13,    91,   149,    47,
   284,   128,   195,   161,   155,    93,    46,   113,   308,   288,
   198,   197,   289,   191,   306,   167,   166,   147,   266,   173,
   171,   172,   265,   190,   295,    58,   161,   165,   153,   173,
   171,   172,   102,   305,    58,    58,   104,   304,    25,    33,
    31,   167,   166,    30,    97,   302,   271,   101,   303,    77,
    33,    31,   116,    21,   214,   100,   292,    58,    22,   192,
   174,   298,   285,    58,   104,    26,    12,   123,   167,   166,
    58,   139,   294,   162,    16,   247,   281,   301,   123,    77,
    33,    31,    51,   262,   256,   296,    58,    27,   251,    38,
   181,   252,    28,    58,   122,   204,    90,   121,    27,   180,
    67,   283,   287,    28,   246,   122,   154,   247,   121,    58,
    70,   269,    79,     1,   206,   268,    58,    17,    70,    33,
    63,   239,    35,   158,    58,   297,    87,   143,    27,    76,
    58,   236,    69,    28,   134,    18,    58,    25,    33,    31,
    69,    32,    30,    57,    58,    10,    25,    33,    31,   215,
    58,    30,    21,    77,    33,    31,   150,    22,    58,    42,
    51,    21,    23,    82,    58,     4,    22,    72,    33,     5,
    58,     3,   125,    16,    21,    77,    33,    31,     8,   244,
   130,   133,    16,    74,   234,    65,    27,    77,    33,    31,
     6,    28,     7,   114,    42,    27,   117,    33,    31,    30,
    28,    30,    27,    42,    92,     2,   249,    28,    30,    21,
   255,    21,   202,    51,    22,    14,    22,   209,    21,    72,
    50,    39,   203,    22,    27,   282,   148,    30,   193,    28,
    16,   278,    16,    83,     9,   183,    27,    21,   182,    16,
    89,    28,    22,   163,   105,    51,   164,   104,   140,   118,
    11,   127,    39,     0,     0,     0,    41,   135,    16,    72,
     0,     0,     0,     0,    98,     0,    52,     0,     0,     0,
     0,     0,   115,   150,     0,     0,     0,     0,     0,     0,
     0,     0,     0,   136,   243,     0,     0,    41,     0,     0,
     0,    52,     0,   145,     0,     0,     0,     0,     0,    41,
     0,   176,   179,     0,     0,     0,     0,   152,     0,     0,
     0,     0,   184,   275,     0,   157,     0,     0,     0,     0,
     0,     0,   202,     0,   199,   136,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,   212,     0,     0,
     0,     0,    41,     0,    52,     0,     0,   217,   218,     0,
    41,     0,     0,     0,    39,     0,     0,     0,    72,   176,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,    50,     0,   232,   233,   248,     0,    41,
     0,     0,     0,     0,     0,   257,     0,    52,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    39,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,    52,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,    41,     0,     0,     0,     0,
     0,   274 };
yytabelem yypact[]={

   149,-10000000,   119,  -102,  -225,   162,  -211,   -24,  -226,   207,
-10000000,-10000000,-10000000,-10000000,  -147,   140,   -26,-10000000,  -127,   101,
-10000000,   -45,   320,  -100,-10000000,   121,-10000000,    82,   332,   190,
   156,-10000000,   -47,-10000000,-10000000,   -88,   162,   -45,    75,  -147,
   206,-10000000,  -132,   -30,   195,   -45,-10000000,   190,   162,  -264,
   274,   266,   251,    95,   463,-10000000,   -89,   -32,   190,   190,
   190,   190,   190,-10000000,-10000000,   223,-10000000,   199,    52,   -33,
-10000000,   175,   -47,    52,  -196,    87,    52,   170,   140,   190,
  -321,  -115,   178,   215,-10000000,-10000000,   190,   122,   197,-10000000,
   -44,   162,   247,   206,   326,-10000000,   205,-10000000,  -149,   162,
  -162,-10000000,-10000000,-10000000,   190,-10000000,   190,-10000000,-10000000,  -321,
  -321,   -29,   -29,   320,   462,   246,-10000000,   -95,  -213,   -28,
    41,    52,   319,   310,-10000000,-10000000,-10000000,   454,-10000000,   -28,
  -213,    52,   190,  -238,   190,  -230,   242,     7,   162,  -121,
   203,-10000000,     0,    52,-10000000,   193,   225,   -45,   109,-10000000,
   173,   209,  -147,  -162,   -36,-10000000,    52,  -147,   -42,   107,
    12,   107,-10000000,-10000000,   -54,-10000000,    52,    52,   190,   190,
   190,   190,   190,   190,  -310,  -240,    14,   -38,    41,-10000000,
   206,   206,    52,-10000000,  -257,    81,   190,   190,    73,   190,
-10000000,   190,   190,   213,   162,-10000000,   190,-10000000,-10000000,   -13,
-10000000,   323,-10000000,   113,-10000000,-10000000,    52,   -44,   190,   307,
  -287,   -43,  -213,   304,    52,-10000000,   -50,-10000000,  -277,   107,
   107,   107,   107,   107,   107,   190,  -295,   303,   -58,-10000000,
   -75,-10000000,   241,   237,-10000000,   190,-10000000,    67,    61,   190,
   107,    -6,    53,   213,-10000000,-10000000,   206,   -44,   -39,-10000000,
   107,   191,  -126,   -79,   296,-10000000,   -44,  -213,    46,   190,
   282,   -59,   190,-10000000,-10000000,-10000000,-10000000,   107,   190,-10000000,
   107,   190,-10000000,-10000000,   184,-10000000,-10000000,-10000000,  -308,   276,
-10000000,   -64,   291,   190,    30,   190,-10000000,   280,   107,    23,
-10000000,   -84,   -45,   264,-10000000,   107,   190,   252,-10000000,-10000000,
-10000000,   233,-10000000,   -68,   107,-10000000,-10000000,   227,-10000000 };
yytabelem yypgo[]={

     0,   440,     0,    99,   207,   248,   228,   511,   261,   475,
   493,   256,   242,   508,   325,   486,   258,   238,   477,   470,
   244,   453,   219,   445,   443,   441,   401,    44,   422,   395,
   349,   464,   510,   326,   377,   373,   383,   320 };
yytabelem yyr1[]={

     0,    35,    35,    35,    35,    35,    35,    35,    35,    35,
    35,    35,    35,    35,    34,    23,    23,    22,    22,    21,
    21,    12,    12,    12,    13,    13,    10,    10,    11,    11,
     5,     5,    18,    18,    18,    18,    18,    17,    17,    17,
    28,    28,    29,    29,    30,    30,    27,    27,     4,     4,
     4,     9,     9,     9,     9,    19,    19,    31,    31,    33,
    33,    33,    33,    33,    33,    32,    32,    32,    15,    15,
    16,     6,     6,    37,    37,    36,    36,     1,     1,     1,
     1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
     1,     1,     1,     1,     1,     1,     1,     3,     3,     8,
     8,     7,     7,    14,    14,    14,    26,    26,    20,    20,
    20,    24,    24,    25,    25,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2 };
yytabelem yyr2[]={

     0,     5,    11,    13,     9,    11,    17,    11,    15,    15,
     7,     9,     5,     3,     5,     3,     7,     7,     7,     3,
     7,     2,     5,     5,     3,     7,     3,     7,     2,     7,
     2,     3,     2,     6,     6,    10,    12,     5,    11,    15,
     9,    11,     2,     7,     3,     7,     3,     7,     3,     7,
     5,     2,     5,    13,     3,     1,     7,     2,     2,     5,
     2,     2,     9,    13,     7,    11,     9,     7,     3,     7,
     7,     3,     7,     0,     2,     0,     2,     7,     7,     7,
     7,     7,     7,    13,    15,    13,    15,     9,    11,     7,
     9,     7,     7,     7,     5,     9,     9,     3,     7,     2,
     2,     3,     7,     3,     7,     7,     3,     5,     3,     3,
     5,     9,    11,     9,    11,     2,     9,     3,    11,    13,
    17,    13,    13,     7,     7,     7,     5,     7,     7,     7,
     7,     7 };
yytabelem yychk[]={

-10000000,   -35,   -31,   282,   276,   280,   301,   303,   289,   -10,
   256,   -32,   -33,   -11,    -9,    40,   293,   -34,   -29,    -2,
    -4,   272,   277,   -28,   -20,   257,   -14,   306,   311,    45,
   262,   259,   -26,   258,    59,   263,   266,   278,   -30,    -9,
    40,   -32,   257,   293,   257,   293,    59,    44,   304,   305,
   -31,   -33,   -32,    -2,    -3,   257,   268,   -36,   323,    42,
    47,    43,    45,   279,    -4,   -23,   -22,    40,   265,   279,
   257,    46,   -26,    40,   -24,    -2,   307,   257,    40,    40,
    -2,    -5,   267,   -10,    42,   258,   264,   -30,   -27,    -4,
   281,    44,   -31,    40,   257,    59,   -27,   -11,    -9,   304,
    41,    41,    41,    41,    44,    41,   264,   257,    -2,    -2,
    -2,    -2,    -2,    44,   -21,   -31,   -20,   257,    -1,    -2,
    40,   316,   313,   286,   257,   257,    42,    -7,    -8,    -2,
    -1,   307,   309,   -25,   307,    -1,   -31,    -2,   266,    -5,
   -13,   -12,    -2,   265,    59,   -31,    40,    44,   -15,   -16,
   -14,   257,    -9,    41,    40,    59,   288,    -9,   -36,    -2,
    -3,    -2,   -22,    41,    44,    41,   315,   314,    61,    60,
    62,   318,   319,   317,   -37,   260,    -1,    -2,    40,    -1,
    40,    40,    44,    41,    -1,    -2,   307,   309,    -2,   308,
    41,   266,   312,   -30,   266,    59,    44,   271,   270,    -1,
    59,    -6,   -14,   257,    -4,    59,   265,    44,    61,   -18,
   -17,   257,    -1,   257,   302,   -20,   257,    -1,    -1,    -2,
    -2,    -2,    -2,    -2,    -2,   320,   316,   321,   322,   261,
   316,    41,   -31,   -31,    -8,   308,   310,    -2,    -2,   308,
    -2,    -2,    -2,   -30,   -12,    59,    41,    44,    -1,   -16,
    -2,    41,    44,   316,   257,   -19,    40,    -1,    -2,   320,
   321,   322,    40,   258,   261,    41,    41,    -2,   308,   310,
    -2,   312,    41,    41,   -31,   -14,    59,    59,   -17,   286,
   261,    40,    -6,   315,    -2,    40,   258,    -3,    -2,    -2,
    59,   316,    40,   259,    41,    -2,   315,    -3,    41,    41,
   261,   -27,    41,    44,    -2,    41,    41,   259,    41 };
yytabelem yydef[]={

     0,    -2,     0,     0,     0,     0,     0,     0,     0,     0,
    13,    -2,    58,    26,     0,     0,     0,    60,    61,    -2,
    51,     0,     0,    42,   115,    -2,   117,     0,     0,     0,
     0,   108,   109,   106,     1,     0,     0,     0,     0,    44,
     0,    54,    48,     0,     0,     0,    12,     0,     0,     0,
     0,     0,    54,    97,     0,    59,     0,     0,     0,     0,
     0,     0,     0,    76,    52,    14,    15,     0,     0,     0,
    50,     0,   110,     0,     0,     0,     0,   103,     0,     0,
   126,     0,     0,    30,    31,   107,     0,     0,     0,    46,
     0,     0,     0,     0,     0,    10,     0,    27,     0,     0,
    -2,    64,    67,   123,     0,   124,     0,    29,   127,   128,
   129,   130,   131,     0,     0,     0,    19,    48,    43,    73,
     0,     0,     0,     0,    49,   104,   105,     0,   101,    -2,
   100,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,    24,    21,     0,     4,     0,     0,     0,     0,    68,
     0,   103,    45,    75,     0,    11,     0,    66,     0,    98,
    62,    97,    16,    17,     0,    18,     0,     0,     0,     0,
     0,     0,     0,     0,     0,    74,     0,    -2,     0,    94,
     0,     0,     0,   116,     0,     0,     0,     0,     0,     0,
   125,     0,     0,    40,     0,     2,     0,    22,    23,     0,
     5,     0,    71,    -2,    47,     7,     0,     0,     0,     0,
    32,     0,    65,    55,     0,    20,     0,    92,    93,    77,
    78,    79,    80,    81,    82,     0,     0,     0,     0,    89,
     0,    91,     0,     0,   102,     0,   118,     0,     0,     0,
   111,     0,     0,    41,    25,     3,     0,     0,     0,    69,
    70,     0,     0,     0,    37,    53,     0,    63,     0,     0,
     0,     0,     0,    87,    90,    95,    96,   112,     0,   119,
   113,     0,   122,   121,     0,    72,     8,     9,    34,     0,
    33,     0,     0,     0,     0,     0,    88,     0,   114,     0,
     6,     0,     0,     0,    56,    83,     0,     0,    85,   120,
    35,     0,    38,     0,    84,    86,    36,     0,    39 };
typedef struct { char *t_name; int t_val; } yytoktype;
#ifndef YYDEBUG
#	define YYDEBUG	0	/* don't allow debugging */
#endif

#if YYDEBUG

char * yyreds[] =
{
	"-no such reduction-",
      "start : tabexp ';'",
      "start : tabexp ORDER BY olist ';'",
      "start : DELETE FROM joinlist WHERE logical ';'",
      "start : DELETE FROM joinlist ';'",
      "start : INSERT INTO filelist tabexp ';'",
      "start : INSERT INTO filelist '(' identlist ')' tabexp ';'",
      "start : UPDATE joinlist SET assignlist ';'",
      "start : UPDATE joinlist SET assignlist WHERE logical ';'",
      "start : CREATE TABLE IDENT '(' typelist ')' ';'",
      "start : USE IDENT ';'",
      "start : DROP TABLE filelist ';'",
      "start : alist ';'",
      "start : error",
      "values : VALUES rowlist",
      "rowlist : rowval",
      "rowlist : rowlist ',' rowval",
      "rowval : '(' atomlist ')'",
      "rowval : '(' tabexp ')'",
      "atomlist : atom",
      "atomlist : atomlist ',' atom",
      "order : expr",
      "order : expr ASC",
      "order : expr DESC",
      "olist : order",
      "olist : olist ',' order",
      "alist : alias",
      "alist : alist ',' alias",
      "alias : expr",
      "alias : expr maybeAS IDENT",
      "fldlist : alist",
      "fldlist : '*'",
      "typelist : type",
      "typelist : type NOT NUL",
      "typelist : typelist ',' type",
      "typelist : typelist ',' type NOT NUL",
      "typelist : typelist ',' UNIQUE '(' filelist ')'",
      "type : IDENT IDENT",
      "type : IDENT IDENT '(' CONST ')'",
      "type : IDENT IDENT '(' CONST ',' CONST ')'",
      "join : SELECT fldlist FROM joinlist",
      "join : SELECT DISTINCT fldlist FROM joinlist",
      "where : join",
      "where : join WHERE logical",
      "joinlist : table",
      "joinlist : joinlist ',' table",
      "filelist : file",
      "filelist : filelist ',' file",
      "file : IDENT",
      "file : IDENT AS IDENT",
      "file : IDENT IDENT",
      "table : file",
      "table : OUTER file",
      "table : '(' tabexp ')' maybeAS IDENT collist",
      "table : joinexp",
      "collist : /* empty */",
      "collist : '(' identlist ')'",
      "tabexp : joinexp",
      "tabexp : nonjoin",
      "nonjoin : TABLE IDENT",
      "nonjoin : values",
      "nonjoin : where",
      "nonjoin : where GROUP BY exprlist",
      "nonjoin : where GROUP BY exprlist HAVING logical",
      "nonjoin : '(' nonjoin ')'",
      "joinexp : table JOIN table ON logical",
      "joinexp : table CROSS JOIN table",
      "joinexp : '(' joinexp ')'",
      "assignlist : assign",
      "assignlist : assignlist ',' assign",
      "assign : ident '=' expr",
      "identlist : ident",
      "identlist : identlist ',' ident",
      "maybeis : /* empty */",
      "maybeis : IS",
      "maybeAS : /* empty */",
      "maybeAS : AS",
      "logical : expr '=' expr",
      "logical : expr '<' expr",
      "logical : expr '>' expr",
      "logical : expr LE expr",
      "logical : expr GE expr",
      "logical : expr NE expr",
      "logical : expr maybeis BETWEEN expr AND expr",
      "logical : expr maybeis NOT BETWEEN expr AND expr",
      "logical : expr maybeis IN '(' exprlist ')'",
      "logical : expr maybeis NOT IN '(' exprlist ')'",
      "logical : expr maybeis LIKE STRING",
      "logical : expr maybeis NOT LIKE STRING",
      "logical : expr IS NUL",
      "logical : expr IS NOT NUL",
      "logical : '(' logical ')'",
      "logical : logical AND logical",
      "logical : logical OR logical",
      "logical : NOT logical",
      "logical : EXISTS '(' tabexp ')'",
      "logical : UNIQUE '(' tabexp ')'",
      "exprlist : expr",
      "exprlist : exprlist ',' expr",
      "arg : expr",
      "arg : logical",
      "arglist : arg",
      "arglist : arglist ',' arg",
      "ident : IDENT",
      "ident : IDENT '.' IDENT",
      "ident : IDENT '.' '*'",
      "string : STRING",
      "string : string STRING",
      "atom : CONST",
      "atom : string",
      "atom : IDENT string",
      "whenlist : WHEN logical THEN expr",
      "whenlist : whenlist WHEN logical THEN expr",
      "whenxlist : WHEN expr THEN expr",
      "whenxlist : whenxlist WHEN expr THEN expr",
      "expr : atom",
      "expr : IDENT '(' arglist ')'",
      "expr : ident",
      "expr : CASE whenlist ELSE expr END",
      "expr : CASE expr whenxlist ELSE expr END",
      "expr : SUBSTRING '(' expr FROM expr FOR expr ')'",
      "expr : SUBSTRING '(' expr FOR expr ')'",
      "expr : SUBSTRING '(' expr FROM expr ')'",
      "expr : '(' expr ')'",
      "expr : '(' exprlist ')'",
      "expr : '(' tabexp ')'",
      "expr : '-' expr",
      "expr : expr CONCAT expr",
      "expr : expr '*' expr",
      "expr : expr '/' expr",
      "expr : expr '+' expr",
      "expr : expr '-' expr",
};
yytoktype yytoks[] =
{
	"IDENT",	257,
	"STRING",	258,
	"CONST",	259,
	"IS",	260,
	"NUL",	261,
	"SELECT",	262,
	"ORDER",	263,
	"BY",	264,
	"WHERE",	265,
	"FROM",	266,
	"DISTINCT",	267,
	"GROUP",	268,
	"ERROR",	269,
	"DESC",	270,
	"ASC",	271,
	"OUTER",	272,
	"ALL",	273,
	"ANY",	274,
	"CONNECT",	275,
	"INSERT",	276,
	"VALUES",	277,
	"INTO",	278,
	"AS",	279,
	"UPDATE",	280,
	"SET",	281,
	"DELETE",	282,
	"ALTER",	283,
	"MODIFY",	284,
	"ADD",	285,
	"UNIQUE",	286,
	"INDEX",	287,
	"ON",	288,
	"DROP",	289,
	"RENAME",	290,
	"TO",	291,
	"PRIOR",	292,
	"TABLE",	293,
	"VIEW",	294,
	"START",	295,
	"WITH",	296,
	"CLUSTER",	297,
	"UNION",	298,
	"INTERSECT",	299,
	"MINUS",	300,
	"CREATE",	301,
	"HAVING",	302,
	"USE",	303,
	"JOIN",	304,
	"CROSS",	305,
	"CASE",	306,
	"WHEN",	307,
	"THEN",	308,
	"ELSE",	309,
	"END",	310,
	"SUBSTRING",	311,
	"FOR",	312,
	"EXISTS",	313,
	"OR",	314,
	"AND",	315,
	"NOT",	316,
	"'='",	61,
	"'>'",	62,
	"'<'",	60,
	"NE",	317,
	"LE",	318,
	"GE",	319,
	"BETWEEN",	320,
	"IN",	321,
	"LIKE",	322,
	"'+'",	43,
	"'-'",	45,
	"'*'",	42,
	"'/'",	47,
	"CONCAT",	323,
	"'.'",	46,
	"';'",	59,
	"'('",	40,
	"')'",	41,
	"','",	44,
	"-unknown-",	-1	/* ends search */
};
#endif /* YYDEBUG */

/* @(#)27       1.7.1.4  src/bos/usr/ccs/bin/yacc/yaccpar, cmdlang, bos41C, s9548C 11/28/95 13:48:59 */
/*
 * COMPONENT_NAME: (CMDLANG) Language Utilities
 *
 * FUNCTIONS: yyparse
 * ORIGINS: 3
 */
/*
** Skeleton parser driver for yacc output
*/

/*
** yacc user known macros and defines
*/
#ifdef YYSPLIT
#   define YYERROR      return(-2)
#else
#   define YYERROR      goto yyerrlab
#endif
#ifdef YACC_MSG
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE
#endif
#include <nl_types.h>
nl_catd yyusercatd;
#endif
#define YYACCEPT        return(0)
#define YYABORT         return(1)
#ifndef YACC_MSG
#define YYBACKUP( newtoken, newvalue )\
{\
        if ( yychar >= 0 || ( yyr2[ yytmp ] >> 1 ) != 1 )\
        {\
                yyerror( "syntax error - cannot backup" );\
                YYERROR;\
        }\
        yychar = newtoken;\
        yystate = *yyps;\
        yylval = newvalue;\
        goto yynewstate;\
}
#else
#define YYBACKUP( newtoken, newvalue )\
{\
        if ( yychar >= 0 || ( yyr2[ yytmp ] >> 1 ) != 1 )\
        {\
                yyusercatd=catopen("yacc_user.cat", NL_CAT_LOCALE);\
                yyerror(catgets(yyusercatd,1,1,"syntax error - cannot backup" ));\
                YYERROR;\
        }\
        yychar = newtoken;\
        yystate = *yyps;\
        yylval = newvalue;\
        goto yynewstate;\
}
#endif
#define YYRECOVERING()  (!!yyerrflag)
#ifndef YYDEBUG
#       define YYDEBUG  1       /* make debugging available */
#endif

/*
** user known globals
*/
int yydebug;                    /* set to 1 to get debugging */

/*
** driver internal defines
*/
#define YYFLAG          (-10000000)

#ifdef YYSPLIT
#   define YYSCODE { \
                        extern int (*_yyf[])(); \
                        register int yyret; \
                        if (_yyf[yytmp]) \
                            if ((yyret=(*_yyf[yytmp])()) == -2) \
                                    goto yyerrlab; \
                                else if (yyret>=0) return(yyret); \
                   }
#endif

/*
** global variables used by the parser
*/
YYSTYPE yyv[ YYMAXDEPTH ];      /* value stack */
int yys[ YYMAXDEPTH ];          /* state stack */

YYSTYPE *yypv;                  /* top of value stack */
YYSTYPE *yypvt;                 /* top of value stack for $vars */
int *yyps;                      /* top of state stack */

int yystate;                    /* current state */
int yytmp;                      /* extra var (lasts between blocks) */

int yynerrs;                    /* number of errors */
int yyerrflag;                  /* error recovery flag */
int yychar;                     /* current input token number */

#ifdef __cplusplus
 #ifdef _CPP_IOSTREAMS
  #include <iostream.h>
  extern void yyerror (char *); /* error message routine -- iostream version */
 #else
  #include <stdio.h>
  extern "C" void yyerror (char *); /* error message routine -- stdio version */
 #endif /* _CPP_IOSTREAMS */
 extern "C" int yylex(void);        /* return the next token */
#endif /* __cplusplus */


/*
** yyparse - return 0 if worked, 1 if syntax error not recovered from
*/
#ifdef __cplusplus
extern "C"
#endif /* __cplusplus */
int
yyparse()
{
        /*
        ** Initialize externals - yyparse may be called more than once
        */
        yypv = &yyv[-1];
        yyps = &yys[-1];
        yystate = 0;
        yytmp = 0;
        yynerrs = 0;
        yyerrflag = 0;
        yychar = -1;
#ifdef YACC_MSG
        yyusercatd=catopen("yacc_user.cat", NL_CAT_LOCALE);
#endif
        goto yystack;
        {
                register YYSTYPE *yy_pv;        /* top of value stack */
                register int *yy_ps;            /* top of state stack */
                register int yy_state;          /* current state */
                register int  yy_n;             /* internal state number info */

                /*
                ** get globals into registers.
                ** branch to here only if YYBACKUP was called.
                */
        yynewstate:
                yy_pv = yypv;
                yy_ps = yyps;
                yy_state = yystate;
                goto yy_newstate;

                /*
                ** get globals into registers.
                ** either we just started, or we just finished a reduction
                */
        yystack:
                yy_pv = yypv;
                yy_ps = yyps;
                yy_state = yystate;

                /*
                ** top of for (;;) loop while no reductions done
                */
        yy_stack:
                /*
                ** put a state and value onto the stacks
                */
#if YYDEBUG
                /*
                ** if debugging, look up token value in list of value vs.
                ** name pairs.  0 and negative (-1) are special values.
                ** Note: linear search is used since time is not a real
                ** consideration while debugging.
                */
                if ( yydebug )
                {
                        register int yy_i;

#if defined(__cplusplus) && defined(_CPP_IOSTREAMS)
                        cout << "State " << yy_state << " token ";
                        if ( yychar == 0 )
                                cout << "end-of-file" << endl;
                        else if ( yychar < 0 )
                                cout << "-none-" << endl;
#else
                        printf( "State %d, token ", yy_state );
                        if ( yychar == 0 )
                                printf( "end-of-file\n" );
                        else if ( yychar < 0 )
                                printf( "-none-\n" );
#endif /* defined(__cplusplus) && defined(_CPP_IOSTREAMS) */
                        else
                        {
                                for ( yy_i = 0; yytoks[yy_i].t_val >= 0;
                                        yy_i++ )
                                {
                                        if ( yytoks[yy_i].t_val == yychar )
                                                break;
                                }
#if defined(__cplusplus) && defined(_CPP_IOSTREAMS)
                                cout << yytoks[yy_i].t_name << endl;
#else
                                printf( "%s\n", yytoks[yy_i].t_name );
#endif /* defined(__cplusplus) && defined(_CPP_IOSTREAMS) */
                        }
                }
#endif /* YYDEBUG */
                if ( ++yy_ps >= &yys[ YYMAXDEPTH ] )    /* room on stack? */
                {
#ifndef YACC_MSG
                        yyerror( "yacc stack overflow" );
#else
                        yyerror(catgets(yyusercatd,1,2,"yacc stack overflow" ));
#endif
                        YYABORT;
                }
                *yy_ps = yy_state;
                *++yy_pv = yyval;

                /*
                ** we have a new state - find out what to do
                */
        yy_newstate:
                if ( ( yy_n = yypact[ yy_state ] ) <= YYFLAG )
                        goto yydefault;         /* simple state */
#if YYDEBUG
                /*
                ** if debugging, need to mark whether new token grabbed
                */
                yytmp = yychar < 0;
#endif
                if ( ( yychar < 0 ) && ( ( yychar = yylex() ) < 0 ) )
                        yychar = 0;             /* reached EOF */
#if YYDEBUG
                if ( yydebug && yytmp )
                {
                        register int yy_i;

#if defined(__cplusplus) && defined(_CPP_IOSTREAMS)
                        cout << "Received token " << endl;
                        if ( yychar == 0 )
                                cout << "end-of-file" << endl;
                        else if ( yychar < 0 )
                                cout << "-none-" << endl;
#else
                        printf( "Received token " );
                        if ( yychar == 0 )
                                printf( "end-of-file\n" );
                        else if ( yychar < 0 )
                                printf( "-none-\n" );
#endif /* defined(__cplusplus) && defined(_CPP_IOSTREAMS) */
                        else
                        {
                                for ( yy_i = 0; yytoks[yy_i].t_val >= 0;
                                        yy_i++ )
                                {
                                        if ( yytoks[yy_i].t_val == yychar )
                                                break;
                                }
#if defined(__cplusplus) && defined(_CPP_IOSTREAMS)
                                cout << yytoks[yy_i].t_name << endl;
#else
                                printf( "%s\n", yytoks[yy_i].t_name );
#endif /* defined(__cplusplus) && defined(_CPP_IOSTREAMS) */
                        }
                }
#endif /* YYDEBUG */
                if ( ( ( yy_n += yychar ) < 0 ) || ( yy_n >= YYLAST ) )
                        goto yydefault;
                if ( yychk[ yy_n = yyact[ yy_n ] ] == yychar )  /*valid shift*/
                {
                        yychar = -1;
                        yyval = yylval;
                        yy_state = yy_n;
                        if ( yyerrflag > 0 )
                                yyerrflag--;
                        goto yy_stack;
                }

        yydefault:
                if ( ( yy_n = yydef[ yy_state ] ) == -2 )
                {
#if YYDEBUG
                        yytmp = yychar < 0;
#endif
                        if ( ( yychar < 0 ) && ( ( yychar = yylex() ) < 0 ) )
                                yychar = 0;             /* reached EOF */
#if YYDEBUG
                        if ( yydebug && yytmp )
                        {
                                register int yy_i;

#if defined(__cplusplus) && defined(_CPP_IOSTREAMS)
                                cout << "Received token " << endl;
                                if ( yychar == 0 )
                                        cout << "end-of-file" << endl;
                                else if ( yychar < 0 )
                                        cout << "-none-" << endl;
#else
                                printf( "Received token " );
                                if ( yychar == 0 )
                                        printf( "end-of-file\n" );
                                else if ( yychar < 0 )
                                        printf( "-none-\n" );
#endif /* defined(__cplusplus) && defined(_CPP_IOSTREAMS) */
                                else
                                {
                                        for ( yy_i = 0;
                                                yytoks[yy_i].t_val >= 0;
                                                yy_i++ )
                                        {
                                                if ( yytoks[yy_i].t_val
                                                        == yychar )
                                                {
                                                        break;
                                                }
                                        }
#if defined(__cplusplus) && defined(_CPP_IOSTREAMS)
                                        cout << yytoks[yy_i].t_name << endl;
#else
                                        printf( "%s\n", yytoks[yy_i].t_name );
#endif /* defined(__cplusplus) && defined(_CPP_IOSTREAMS) */
                                }
                        }
#endif /* YYDEBUG */
                        /*
                        ** look through exception table
                        */
                        {
                                register int *yyxi = yyexca;

                                while ( ( *yyxi != -1 ) ||
                                        ( yyxi[1] != yy_state ) )
                                {
                                        yyxi += 2;
                                }
                                while ( ( *(yyxi += 2) >= 0 ) &&
                                        ( *yyxi != yychar ) )
                                        ;
                                if ( ( yy_n = yyxi[1] ) < 0 )
                                        YYACCEPT;
                        }
                }

                /*
                ** check for syntax error
                */
                if ( yy_n == 0 )        /* have an error */
                {
                        /* no worry about speed here! */
                        switch ( yyerrflag )
                        {
                        case 0:         /* new error */
#ifndef YACC_MSG
                                yyerror( "syntax error" );
#else
                                yyerror(catgets(yyusercatd,1,3,"syntax error" ));
#endif
                                goto skip_init;
                        yyerrlab:
                                /*
                                ** get globals into registers.
                                ** we have a user generated syntax type error
                                */
                                yy_pv = yypv;
                                yy_ps = yyps;
                                yy_state = yystate;
                                yynerrs++;
                        skip_init:
                        case 1:
                        case 2:         /* incompletely recovered error */
                                        /* try again... */
                                yyerrflag = 3;
                                /*
                                ** find state where "error" is a legal
                                ** shift action
                                */
                                while ( yy_ps >= yys )
                                {
                                        yy_n = yypact[ *yy_ps ] + YYERRCODE;
                                        if ( yy_n >= 0 && yy_n < YYLAST &&
                                                yychk[yyact[yy_n]] == YYERRCODE)                                        {
                                                /*
                                                ** simulate shift of "error"
                                                */
                                                yy_state = yyact[ yy_n ];
                                                goto yy_stack;
                                        }
                                        /*
                                        ** current state has no shift on
                                        ** "error", pop stack
                                        */
#if YYDEBUG
                                        if ( yydebug )
#if defined(__cplusplus) && defined(_CPP_IOSTREAMS)
                                            cout << "Error recovery pops state "
                                                 << (*yy_ps)
                                                 << ", uncovers state "
                                                 << yy_ps[-1] << endl;
#else
#       define _POP_ "Error recovery pops state %d, uncovers state %d\n"
                                                printf( _POP_, *yy_ps,
                                                        yy_ps[-1] );
#       undef _POP_
#endif /* defined(__cplusplus) && defined(_CPP_IOSTREAMS) */
#endif
                                        yy_ps--;
                                        yy_pv--;
                                }
                                /*
                                ** there is no state on stack with "error" as
                                ** a valid shift.  give up.
                                */
                                YYABORT;
                        case 3:         /* no shift yet; eat a token */
#if YYDEBUG
                                /*
                                ** if debugging, look up token in list of
                                ** pairs.  0 and negative shouldn't occur,
                                ** but since timing doesn't matter when
                                ** debugging, it doesn't hurt to leave the
                                ** tests here.
                                */
                                if ( yydebug )
                                {
                                        register int yy_i;

#if defined(__cplusplus) && defined(_CPP_IOSTREAMS)
                                        cout << "Error recovery discards ";
                                        if ( yychar == 0 )
                                            cout << "token end-of-file" << endl;
                                        else if ( yychar < 0 )
                                            cout << "token -none-" << endl;
#else
                                        printf( "Error recovery discards " );
                                        if ( yychar == 0 )
                                                printf( "token end-of-file\n" );
                                        else if ( yychar < 0 )
                                                printf( "token -none-\n" );
#endif /* defined(__cplusplus) && defined(_CPP_IOSTREAMS) */
                                        else
                                        {
                                                for ( yy_i = 0;
                                                        yytoks[yy_i].t_val >= 0;
                                                        yy_i++ )
                                                {
                                                        if ( yytoks[yy_i].t_val
                                                                == yychar )
                                                        {
                                                                break;
                                                        }
                                                }
#if defined(__cplusplus) && defined(_CPP_IOSTREAMS)
                                                cout << "token " <<
                                                    yytoks[yy_i].t_name <<
                                                    endl;
#else
                                                printf( "token %s\n",
                                                        yytoks[yy_i].t_name );
#endif /* defined(__cplusplus) && defined(_CPP_IOSTREAMS) */
                                        }
                                }
#endif /* YYDEBUG */
                                if ( yychar == 0 )      /* reached EOF. quit */
                                        YYABORT;
                                yychar = -1;
                                goto yy_newstate;
                        }
                }/* end if ( yy_n == 0 ) */
                /*
                ** reduction by production yy_n
                ** put stack tops, etc. so things right after switch
                */
#if YYDEBUG
                /*
                ** if debugging, print the string that is the user's
                ** specification of the reduction which is just about
                ** to be done.
                */
                if ( yydebug )
#if defined(__cplusplus) && defined(_CPP_IOSTREAMS)
                        cout << "Reduce by (" << yy_n << ") \"" <<
                            yyreds[ yy_n ] << "\"\n";
#else
                        printf( "Reduce by (%d) \"%s\"\n",
                                yy_n, yyreds[ yy_n ] );
#endif /* defined(__cplusplus) && defined(_CPP_IOSTREAMS) */
#endif
                yytmp = yy_n;                   /* value to switch over */
                yypvt = yy_pv;                  /* $vars top of value stack */
                /*
                ** Look in goto table for next state
                ** Sorry about using yy_state here as temporary
                ** register variable, but why not, if it works...
                ** If yyr2[ yy_n ] doesn't have the low order bit
                ** set, then there is no action to be done for
                ** this reduction.  So, no saving & unsaving of
                ** registers done.  The only difference between the
                ** code just after the if and the body of the if is
                ** the goto yy_stack in the body.  This way the test
                ** can be made before the choice of what to do is needed.
                */
                {
                        /* length of production doubled with extra bit */
                        register int yy_len = yyr2[ yy_n ];

                        if ( !( yy_len & 01 ) )
                        {
                                yy_len >>= 1;
                                yyval = ( yy_pv -= yy_len )[1]; /* $$ = $1 */
                                yy_state = yypgo[ yy_n = yyr1[ yy_n ] ] +
                                        *( yy_ps -= yy_len ) + 1;
                                if ( yy_state >= YYLAST ||
                                        yychk[ yy_state =
                                        yyact[ yy_state ] ] != -yy_n )
                                {
                                        yy_state = yyact[ yypgo[ yy_n ] ];
                                }
                                goto yy_stack;
                        }
                        yy_len >>= 1;
                        yyval = ( yy_pv -= yy_len )[1]; /* $$ = $1 */
                        yy_state = yypgo[ yy_n = yyr1[ yy_n ] ] +
                                *( yy_ps -= yy_len ) + 1;
                        if ( yy_state >= YYLAST ||
                                yychk[ yy_state = yyact[ yy_state ] ] != -yy_n )
                        {
                                yy_state = yyact[ yypgo[ yy_n ] ];
                        }
                }
                                        /* save until reenter driver code */
                yystate = yy_state;
                yyps = yy_ps;
                yypv = yy_pv;
        }
        /*
        ** code supplied by user is placed in this switch
        */

                switch(yytmp){

case 1:{ s.cmd = SQL_SELECT;
		  s.src = yypvt[-1].qry; } /*NOTREACHED*/ break;
case 2:{ s.cmd = SQL_SELECT;
		  s.src = yypvt[-4].qry;
		  yypvt[-4].qry->order_list = yypvt[-1].exp; } /*NOTREACHED*/ break;
case 3:{ s.cmd = SQL_DELETE;
		  s.dst = yypvt[-3].qry;
		  s.dst->where_sql = yypvt[-1].exp; } /*NOTREACHED*/ break;
case 4:{ s.cmd = SQL_DELETE;
		  s.dst = yypvt[-1].qry; } /*NOTREACHED*/ break;
case 5:{ s.cmd = SQL_INSERT;
		  s.dst = yypvt[-2].qry;
		  s.src = yypvt[-1].qry; } /*NOTREACHED*/ break;
case 6:{ s.cmd = SQL_INSERT;
		  s.dst = yypvt[-5].qry;
		  s.dst->select_list = yypvt[-3].exp;
		  s.src = yypvt[-1].qry; } /*NOTREACHED*/ break;
case 7:{ s.cmd = SQL_UPDATE;
		  s.dst = yypvt[-3].qry;
		  s.dst->select_list = yypvt[-1].exp; } /*NOTREACHED*/ break;
case 8:{ s.cmd = SQL_UPDATE;
		  s.dst = yypvt[-5].qry;
		  s.dst->where_sql = yypvt[-1].exp;
		  s.dst->select_list = yypvt[-3].exp; } /*NOTREACHED*/ break;
case 9:{ s.cmd = SQL_CREATE;
		  s.dst = select_init();
		  s.dst->table_list = mkname(yypvt[-4].name,yypvt[-4].name);
		  s.val = yypvt[-2].exp;
		} /*NOTREACHED*/ break;
case 10:{ s.cmd = SQL_USE;
		  s.val = mkname(yypvt[-1].name,yypvt[-1].name);
		} /*NOTREACHED*/ break;
case 11:{ s.cmd = SQL_DROP;
		  s.dst = yypvt[-1].qry; } /*NOTREACHED*/ break;
case 12:{ s.cmd = SQL_SELECT;
		  s.src = select_init();
		  addlist(s.src->table_list = mklist(),
			mkname("system","system"));
		  s.src->select_list = yypvt[-1].exp; } /*NOTREACHED*/ break;
case 13:{ s.cmd = SQL_ERROR; } /*NOTREACHED*/ break;
case 14:{ yyval.qry = select_init();
		  yyval.qry->select_list = yypvt[-0].exp; } /*NOTREACHED*/ break;
case 15:{ addlist(yyval.exp = mklist(),yypvt[-0].exp); } /*NOTREACHED*/ break;
case 16:{ addlist(yyval.exp,yypvt[-0].exp); } /*NOTREACHED*/ break;
case 17:{ yyval.exp = yypvt[-1].exp; } /*NOTREACHED*/ break;
case 18:{ yyval.exp = mksql(EXQUERY);
		  yyval.exp->u.q.qry = yypvt[-1].qry;
		  yyval.exp->u.q.name = 0;
		} /*NOTREACHED*/ break;
case 19:{ addlist(yyval.exp = mklist(),yypvt[-0].exp); } /*NOTREACHED*/ break;
case 20:{ addlist(yyval.exp,yypvt[-0].exp); } /*NOTREACHED*/ break;
case 22:{ yyval.exp = yypvt[-1].exp; } /*NOTREACHED*/ break;
case 23:{ yyval.exp = mkunop(EXDESC,yypvt[-1].exp); } /*NOTREACHED*/ break;
case 24:{ addlist(yyval.exp = mklist(),yypvt[-0].exp); } /*NOTREACHED*/ break;
case 25:{ addlist(yyval.exp = yypvt[-2].exp,yypvt[-0].exp); } /*NOTREACHED*/ break;
case 26:{ addlist(yyval.exp = mklist(),yypvt[-0].exp); } /*NOTREACHED*/ break;
case 27:{ addlist(yyval.exp = yypvt[-2].exp,yypvt[-0].exp); } /*NOTREACHED*/ break;
case 29:{ yyval.exp = mkalias(yypvt[-0].name,yypvt[-2].exp); } /*NOTREACHED*/ break;
case 31:{ yyval.exp = 0; } /*NOTREACHED*/ break;
case 37:{ addlist(yyval.exp = mklist(),mkname(yypvt[-1].name,yypvt[-0].name)); } /*NOTREACHED*/ break;
case 38:{ if (yypvt[-1].num.fix) YYERROR;
		  addlist(yyval.exp = mklist(),mkname(yypvt[-4].name,yypvt[-3].name));
		  addlist(yyval.exp,mkconst(&yypvt[-1].num)); } /*NOTREACHED*/ break;
case 39:{ if (yypvt[-3].num.fix || yypvt[-1].num.fix) YYERROR;
		  addlist(yyval.exp = mklist(),mkname(yypvt[-6].name,yypvt[-5].name));
		  addlist(yyval.exp,mkconst(&yypvt[-3].num));
		  addlist(yyval.exp,mkconst(&yypvt[-1].num)); } /*NOTREACHED*/ break;
case 40:{ yyval.qry = yypvt[-0].qry;
		  yyval.qry->select_list = yypvt[-2].exp; } /*NOTREACHED*/ break;
case 41:{ yyval.qry = yypvt[-0].qry;
		  yyval.qry->select_list = yypvt[-2].exp;
		  yyval.qry->distinct = 1; } /*NOTREACHED*/ break;
case 43:{ yyval.qry = yypvt[-2].qry; yyval.qry->where_sql = yypvt[-0].exp; } /*NOTREACHED*/ break;
case 44:{ yyval.qry = select_init();
		  addlist(yyval.qry->table_list = mklist(),yypvt[-0].exp); } /*NOTREACHED*/ break;
case 45:{ yyval.qry = yypvt[-2].qry;
		  addlist(yyval.qry->table_list,yypvt[-0].exp); } /*NOTREACHED*/ break;
case 46:{ yyval.qry = select_init();
		  addlist(yyval.qry->table_list = mklist(),yypvt[-0].exp); } /*NOTREACHED*/ break;
case 47:{ yyval.qry = yypvt[-2].qry;
		  addlist(yyval.qry->table_list,yypvt[-0].exp); } /*NOTREACHED*/ break;
case 48:{ yyval.exp = mkname(yypvt[-0].name,yypvt[-0].name); } /*NOTREACHED*/ break;
case 49:{ yyval.exp = mkname(yypvt[-2].name,yypvt[-0].name); } /*NOTREACHED*/ break;
case 50:{ yyval.exp = mkname(yypvt[-1].name,yypvt[-0].name); } /*NOTREACHED*/ break;
case 52:{ yyval.exp = mkunop(EXISNUL,yypvt[-0].exp); } /*NOTREACHED*/ break;
case 53:{ yyval.exp = mksql(EXQUERY);
		  yyval.exp->u.q.qry = yypvt[-4].qry;
		  yyval.exp->u.q.name = yypvt[-1].name;
		  if (yypvt[-0].exp)
		    yyval.exp = mkbinop(yyval.exp,EXEQ,yypvt[-0].exp);	/* assign new column names */
		} /*NOTREACHED*/ break;
case 54:{ yyval.exp = mksql(EXQUERY);
		  yyval.exp->u.q.qry = yypvt[-0].qry;
		  yyval.exp->u.q.name = 0;
		} /*NOTREACHED*/ break;
case 55:{ yyval.exp = 0; } /*NOTREACHED*/ break;
case 56:{ yyval.exp = yypvt[-1].exp; } /*NOTREACHED*/ break;
case 59:{ yyval.qry = select_init();
		  addlist(yyval.qry->table_list = mklist(),mkname(yypvt[-0].name,yypvt[-0].name)); } /*NOTREACHED*/ break;
case 62:{ yyval.qry = yypvt[-3].qry; yyval.qry->group_list = yypvt[-0].exp; } /*NOTREACHED*/ break;
case 63:{ yyval.qry = yypvt[-5].qry; yyval.qry->group_list = yypvt[-2].exp; yyval.qry->having_sql = yypvt[-0].exp; } /*NOTREACHED*/ break;
case 64:{ yyval.qry = yypvt[-1].qry; } /*NOTREACHED*/ break;
case 65:{ yyval.qry = select_init();
		  yyval.qry->table_list = mklist();
		  addlist(yyval.qry->table_list,yypvt[-4].exp);
		  addlist(yyval.qry->table_list,yypvt[-2].exp);
		  yyval.qry->where_sql = yypvt[-0].exp;
		} /*NOTREACHED*/ break;
case 66:{ yyval.qry = select_init();
		  yyval.qry->table_list = mklist();
		  addlist(yyval.qry->table_list,yypvt[-3].exp);
		  addlist(yyval.qry->table_list,yypvt[-0].exp);
		} /*NOTREACHED*/ break;
case 67:{ yyval.qry = yypvt[-1].qry; } /*NOTREACHED*/ break;
case 68:{ addlist(yyval.exp = mklist(),yypvt[-0].exp); } /*NOTREACHED*/ break;
case 69:{ addlist(yyval.exp = yypvt[-2].exp,yypvt[-0].exp); } /*NOTREACHED*/ break;
case 70:{ yyval.exp = mkbinop(yypvt[-2].exp,EXEQ,yypvt[-0].exp); } /*NOTREACHED*/ break;
case 71:{ addlist(yyval.exp = mklist(),yypvt[-0].exp); } /*NOTREACHED*/ break;
case 72:{ addlist(yyval.exp = yypvt[-2].exp,yypvt[-0].exp); } /*NOTREACHED*/ break;
case 77:{ yyval.exp = mkbinop(yypvt[-2].exp,EXEQ,yypvt[-0].exp); } /*NOTREACHED*/ break;
case 78:{ yyval.exp = mkbinop(yypvt[-2].exp,EXLT,yypvt[-0].exp); } /*NOTREACHED*/ break;
case 79:{ yyval.exp = mkbinop(yypvt[-2].exp,EXGT,yypvt[-0].exp); } /*NOTREACHED*/ break;
case 80:{ yyval.exp = mkbinop(yypvt[-2].exp,EXLE,yypvt[-0].exp); } /*NOTREACHED*/ break;
case 81:{ yyval.exp = mkbinop(yypvt[-2].exp,EXGE,yypvt[-0].exp); } /*NOTREACHED*/ break;
case 82:{ yyval.exp = mkbinop(yypvt[-2].exp,EXNE,yypvt[-0].exp); } /*NOTREACHED*/ break;
case 83:{ yyval.exp = mkbinop(mkbinop(sql_eval(yypvt[-5].exp,0),EXGE,yypvt[-2].exp),EXAND,
			 mkbinop(yypvt[-0].exp,EXGE,sql_eval(yypvt[-5].exp,0))); } /*NOTREACHED*/ break;
case 84:{ yyval.exp = mkbinop(mkbinop(yypvt[-2].exp,EXGT,sql_eval(yypvt[-6].exp,0)),EXOR,
			 mkbinop(sql_eval(yypvt[-6].exp,0),EXGT,yypvt[-0].exp)); } /*NOTREACHED*/ break;
case 85:{
		  sql x = yypvt[-1].exp->u.opd[1];
		  yyval.exp = mkbinop(sql_eval(yypvt[-5].exp,0),EXEQ,x->u.opd[0]);
		  while (x = x->u.opd[1])
		    yyval.exp = mkbinop(yyval.exp,EXOR,
			mkbinop(sql_eval(yypvt[-5].exp,0),EXEQ,x->u.opd[0]));
		} /*NOTREACHED*/ break;
case 86:{
		  sql x = yypvt[-1].exp->u.opd[1];
		  yyval.exp = mkbinop(sql_eval(yypvt[-6].exp,0),EXNE,x->u.opd[0]);
		  while (x = x->u.opd[1])
		    yyval.exp = mkbinop(yyval.exp,EXAND,
			mkbinop(sql_eval(yypvt[-6].exp,0),EXNE,x->u.opd[0]));
		} /*NOTREACHED*/ break;
case 87:{ yyval.exp = mkbinop(yypvt[-3].exp,EXLIKE,mkstring(yypvt[-0].name)); } /*NOTREACHED*/ break;
case 88:{ yyval.exp = mkunop(EXNOT,mkbinop(yypvt[-4].exp,EXLIKE,mkstring(yypvt[-0].name))); } /*NOTREACHED*/ break;
case 89:{ yyval.exp = mkunop(EXISNUL,yypvt[-2].exp); } /*NOTREACHED*/ break;
case 90:{ yyval.exp = mkunop(EXNOT,mkunop(EXISNUL,yypvt[-3].exp)); } /*NOTREACHED*/ break;
case 91:{ yyval.exp = yypvt[-1].exp; } /*NOTREACHED*/ break;
case 92:{ yyval.exp = mkbinop(yypvt[-2].exp,EXAND,yypvt[-0].exp); } /*NOTREACHED*/ break;
case 93:{ yyval.exp = mkbinop(yypvt[-2].exp,EXOR,yypvt[-0].exp); } /*NOTREACHED*/ break;
case 94:{ yyval.exp = mkunop(EXNOT,yypvt[-0].exp); } /*NOTREACHED*/ break;
case 95:{ sql q = mksql(EXQUERY);
		  q->u.q.qry = yypvt[-1].qry; q->u.q.name = 0;
		  yyval.exp = mkunop(EXEXIST,q); } /*NOTREACHED*/ break;
case 96:{ sql q = mksql(EXQUERY);
		  q->u.q.qry = yypvt[-1].qry; q->u.q.name = 0;
		  yyval.exp = mkunop(EXUNIQ,q); } /*NOTREACHED*/ break;
case 97:{ addlist(yyval.exp = mklist(),yypvt[-0].exp); } /*NOTREACHED*/ break;
case 98:{ addlist(yyval.exp = yypvt[-2].exp,yypvt[-0].exp); } /*NOTREACHED*/ break;
case 101:{ addlist(yyval.exp = mklist(),yypvt[-0].exp); } /*NOTREACHED*/ break;
case 102:{ addlist(yyval.exp = yypvt[-2].exp,yypvt[-0].exp); } /*NOTREACHED*/ break;
case 103:{ yyval.exp = mkname((const char *)0,yypvt[-0].name); } /*NOTREACHED*/ break;
case 104:{ yyval.exp = mkname(yypvt[-2].name,yypvt[-0].name); } /*NOTREACHED*/ break;
case 105:{ yyval.exp = mkname(yypvt[-2].name,(const char *)0); } /*NOTREACHED*/ break;
case 106:{ yyval.exp = mkstring(yypvt[-0].name); } /*NOTREACHED*/ break;
case 107:{ yyval.exp = mkbinop(yypvt[-1].exp,EXCAT,mkstring(yypvt[-0].name)); } /*NOTREACHED*/ break;
case 108:{ yyval.exp = mkconst(&yypvt[-0].num); } /*NOTREACHED*/ break;
case 109:{ yyval.exp = yypvt[-0].exp; } /*NOTREACHED*/ break;
case 110:{ yyval.exp = mklit(yypvt[-1].name,yypvt[-0].exp); } /*NOTREACHED*/ break;
case 111:{ addlist(yyval.exp = mklist(),yypvt[-2].exp); addlist(yyval.exp,yypvt[-0].exp); } /*NOTREACHED*/ break;
case 112:{ addlist(yyval.exp = yypvt[-4].exp,yypvt[-2].exp); addlist(yyval.exp,yypvt[-0].exp); } /*NOTREACHED*/ break;
case 113:{ addlist(yyval.exp = mklist(),yypvt[-2].exp); addlist(yyval.exp,yypvt[-0].exp); } /*NOTREACHED*/ break;
case 114:{ addlist(yyval.exp = yypvt[-4].exp,yypvt[-2].exp); addlist(yyval.exp,yypvt[-0].exp); } /*NOTREACHED*/ break;
case 116:{ yyval.exp = mkfunc(yypvt[-3].name,yypvt[-1].exp); } /*NOTREACHED*/ break;
case 117:{ yyval.exp = yypvt[-0].exp; } /*NOTREACHED*/ break;
case 118:{ addlist(yypvt[-3].exp,yypvt[-1].exp);
		  yyval.exp = sql_if(yypvt[-3].exp); } /*NOTREACHED*/ break;
case 119:{ sql x = yypvt[-3].exp;
		  while (x = x->u.opd[1]) {
		    x->u.opd[0] = mkbinop(sql_eval(yypvt[-4].exp,0),EXEQ,x->u.opd[0]);
		    x = x->u.opd[1];
		  }
		  addlist(yypvt[-3].exp,yypvt[-1].exp);
		  yyval.exp = sql_if(yypvt[-3].exp); } /*NOTREACHED*/ break;
case 120:{ addlist(yyval.exp = mklist(),yypvt[-5].exp); addlist(yyval.exp,yypvt[-3].exp); addlist(yyval.exp,yypvt[-1].exp);
		  yyval.exp = sql_substr(yyval.exp); } /*NOTREACHED*/ break;
case 121:{ static sconst one = { { 0, 1 } };
		  addlist(yyval.exp = mklist(),yypvt[-3].exp); addlist(yyval.exp,mkconst(&one));
		  addlist(yyval.exp,yypvt[-1].exp); yyval.exp = sql_substr(yyval.exp); } /*NOTREACHED*/ break;
case 122:{ addlist(yyval.exp = mklist(),yypvt[-3].exp); addlist(yyval.exp,yypvt[-1].exp);
		  yyval.exp = sql_substr(yyval.exp); } /*NOTREACHED*/ break;
case 123:{ yyval.exp = yypvt[-1].exp; } /*NOTREACHED*/ break;
case 124:{ yyval.exp = yypvt[-1].exp; } /*NOTREACHED*/ break;
case 125:{ yyval.exp = mksql(EXQUERY); yyval.exp->u.q.qry = yypvt[-1].qry; yyval.exp->u.q.name = 0; } /*NOTREACHED*/ break;
case 126:{ yyval.exp = mkunop(EXNEG,yypvt[-0].exp); } /*NOTREACHED*/ break;
case 127:{ yyval.exp = mkbinop(yypvt[-2].exp,EXCAT,yypvt[-0].exp); } /*NOTREACHED*/ break;
case 128:{ yyval.exp = mkbinop(yypvt[-2].exp,EXMUL,yypvt[-0].exp); } /*NOTREACHED*/ break;
case 129:{ yyval.exp = mkbinop(yypvt[-2].exp,EXDIV,yypvt[-0].exp); } /*NOTREACHED*/ break;
case 130:{ yyval.exp = mkbinop(yypvt[-2].exp,EXADD,yypvt[-0].exp); } /*NOTREACHED*/ break;
case 131:{ yyval.exp = mkbinop(yypvt[-2].exp,EXSUB,yypvt[-0].exp); } /*NOTREACHED*/ break;
}


        goto yystack;           /* reset registers in driver code */
}

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "obmem.h"

static const char *lexptr, *expptr;

static char *quotes(q)
  int q;	/* quote char */
{
  while (*lexptr != (char)q || *++lexptr == (char)q)
    obstack_1grow(sqltree,*lexptr++);
  obstack_1grow(sqltree,0);
  return obstack_finish(sqltree);
}

static int yylex() {
  /* table of reserved words, must be sorted alphabetically */
  static const struct resword {
    char name[10];
    short rid;
  } rtbl[] = {
    { "ADD", ADD }, { "ALL", ALL }, { "ALTER", ALTER }, { "AND", AND },
    { "ANY", ANY }, { "AS", AS }, { "ASC", ASC },
    { "BETWEEN", BETWEEN }, { "BY", BY }, { "CASE", CASE },
    { "CLUSTER", CLUSTER }, { "CONNECT", CONNECT }, { "CREATE", CREATE },
    { "CROSS", CROSS },
    { "DELETE", DELETE }, { "DESC", DESC }, { "DISTINCT", DISTINCT },
    { "DROP", DROP }, { "ELSE", ELSE }, { "END", END }, { "EXISTS", EXISTS },
    { "FOR", FOR }, { "FROM", FROM }, { "GROUP", GROUP },
    { "HAVING", HAVING }, { "IN", IN }, { "INDEX", INDEX },
    { "INSERT", INSERT }, {"INTERSECT", INTERSECT },
    { "INTO", INTO }, { "IS", IS }, { "JOIN", JOIN }, { "LIKE", LIKE },
    { "MINUS", MINUS }, { "MODIFY", MODIFY }, { "NOT", NOT }, { "NULL", NUL },
    { "ON", ON }, { "OR", OR }, { "ORDER", ORDER }, { "OUTER", OUTER },
    { "PRIOR", PRIOR }, { "RENAME", RENAME },
    { "SELECT", SELECT }, { "SET", SET }, { "START", START },
    { "SUBSTRING", SUBSTRING },
    { "TABLE", TABLE }, { "THEN", THEN }, { "TO", TO },
    { "UNION", UNION }, { "UNIQUE", UNIQUE }, { "UPDATE", UPDATE },
    { "USE", USE }, { "VALUES", VALUES }, { "VIEW", VIEW },
    { "WHEN", WHEN }, { "WHERE", WHERE }, { "WITH", WITH },
  };
  static int thistok;
  int lasttok = thistok;
  thistok = 0;
doline:
  while (*lexptr && strchr(" \t\n",*lexptr)) ++lexptr;
  switch (*lexptr) {
  case 0:
    return 0;
  case '"':
    yylval.name = quotes(*lexptr++);
    return IDENT;
  case '\'':
    yylval.name = quotes(*lexptr++);
    return STRING;
  case '/':
    if (lexptr[1] == '/') {
      while (*++lexptr && *lexptr != '\n');
      goto doline;
    }
  case '*': case '+': case '-': case '=':
  case '(': case ')': case ',': case ';':
    return *lexptr++;
  case '.':
    return getconst(&yylval.num,&lexptr) ? CONST : (thistok = '.');
  case '>':
    return (*++lexptr == '=') ? ++lexptr,GE : '>';
  case '<':
    if (*++lexptr == '>') return ++lexptr,NE;
    return (*lexptr == '=') ? ++lexptr,LE : '<';
  case '!':
    return (*++lexptr == '=') ? ++lexptr,NE : ERROR;
  case '^':
    return (*++lexptr == '=') ? ++lexptr,NE : ERROR;
  case '|':
    return (*++lexptr == '|') ? ++lexptr,CONCAT : ERROR;
  }
  if (isalpha(*lexptr) || *lexptr == '#') {
    struct resword *p;
    char *name;
    if (*lexptr == '#') {
      do
	obstack_1grow(sqltree,*lexptr++);
      while (isdigit(*lexptr));
      obstack_1grow(sqltree,0);
      yylval.name = obstack_finish(sqltree);
      return IDENT;
    }
    do
      obstack_1grow(sqltree,toupper(*lexptr++));
    while (isalnum(*lexptr) || *lexptr == '#' || *lexptr == '_');
    obstack_1grow(sqltree,0);
    name = obstack_finish(sqltree);
    if (lasttok != '.') {
      p = bsearch(name,
	    rtbl,sizeof rtbl/sizeof rtbl[0],sizeof rtbl[0],
	    (int cdecl (*)())strcmp);
      if (p) {
	obstack_free(sqltree,name);
	return p->rid;
      }
    }
    if (!mapupper) strlwr(name);
    yylval.name = name;
    return IDENT;
  }
  return (getconst(&yylval.num,&lexptr)) ? CONST : ERROR;
}

void yyerror(const char *s) {
  sql_syntax(s,expptr,lexptr);
}

static struct select *select_init() {
  struct select *s = obstack_alloc(sqltree,sizeof *s);
  s->select_list = 0;
  s->group_list = 0;
  s->having_sql = 0;
  s->order_list = 0;
  s->where_sql = 0;
  s->table_list = 0;
  s->distinct = 0;
  return s;
}

struct sql_stmt *parse_sql(const char *txt) {
  expptr = lexptr = txt;
  s.src = 0;
  s.dst = 0;
  s.val = 0;
  if (yyparse()) return 0;
  return &s;
}
