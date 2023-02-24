/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 1 "parser.y"

    #include <iostream>
    #include <string>
    #include <vector>

    #include "SyntaxTree.hh"

    extern int yylex();
    extern int yyparse();
    extern int yyrestart();
    extern FILE* yyin;

    extern char* yytext;
    extern int line_number;
    extern int column_end_number;
    extern int column_start_number;

    extern SyntaxTree syntax_tree;

    void yyerror(const char *s) {
        std::cerr << s << std::endl;
        std::cerr << "Error at line " << line_number << ": " << column_end_number << std::endl;
        std::cerr << "Error: " << yytext << std::endl;
        std::abort();
    }

#line 98 "parser.cc"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "parser.hh"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_INT = 3,                        /* INT  */
  YYSYMBOL_FLOAT = 4,                      /* FLOAT  */
  YYSYMBOL_VOID = 5,                       /* VOID  */
  YYSYMBOL_CONST = 6,                      /* CONST  */
  YYSYMBOL_IF = 7,                         /* IF  */
  YYSYMBOL_ELSE = 8,                       /* ELSE  */
  YYSYMBOL_WHILE = 9,                      /* WHILE  */
  YYSYMBOL_BREAK = 10,                     /* BREAK  */
  YYSYMBOL_CONTINUE = 11,                  /* CONTINUE  */
  YYSYMBOL_RETURN = 12,                    /* RETURN  */
  YYSYMBOL_FOR = 13,                       /* FOR  */
  YYSYMBOL_Ident = 14,                     /* Ident  */
  YYSYMBOL_IntConst = 15,                  /* IntConst  */
  YYSYMBOL_floatConst = 16,                /* floatConst  */
  YYSYMBOL_ADD = 17,                       /* ADD  */
  YYSYMBOL_SUB = 18,                       /* SUB  */
  YYSYMBOL_MUL = 19,                       /* MUL  */
  YYSYMBOL_DIV = 20,                       /* DIV  */
  YYSYMBOL_MOD = 21,                       /* MOD  */
  YYSYMBOL_LPAREN = 22,                    /* LPAREN  */
  YYSYMBOL_RPAREN = 23,                    /* RPAREN  */
  YYSYMBOL_LBRACKET = 24,                  /* LBRACKET  */
  YYSYMBOL_RBRACKET = 25,                  /* RBRACKET  */
  YYSYMBOL_LBRACE = 26,                    /* LBRACE  */
  YYSYMBOL_RBRACE = 27,                    /* RBRACE  */
  YYSYMBOL_LESS = 28,                      /* LESS  */
  YYSYMBOL_LESS_EQUAL = 29,                /* LESS_EQUAL  */
  YYSYMBOL_GREATER = 30,                   /* GREATER  */
  YYSYMBOL_GREATER_EQUAL = 31,             /* GREATER_EQUAL  */
  YYSYMBOL_EQUAL = 32,                     /* EQUAL  */
  YYSYMBOL_NOT_EQUAL = 33,                 /* NOT_EQUAL  */
  YYSYMBOL_AND = 34,                       /* AND  */
  YYSYMBOL_OR = 35,                        /* OR  */
  YYSYMBOL_NOT = 36,                       /* NOT  */
  YYSYMBOL_ASSIGN = 37,                    /* ASSIGN  */
  YYSYMBOL_ADD_ASSIGN = 38,                /* ADD_ASSIGN  */
  YYSYMBOL_COMMA = 39,                     /* COMMA  */
  YYSYMBOL_SEMICOLON = 40,                 /* SEMICOLON  */
  YYSYMBOL_ERROR = 41,                     /* ERROR  */
  YYSYMBOL_YYACCEPT = 42,                  /* $accept  */
  YYSYMBOL_CompUnit = 43,                  /* CompUnit  */
  YYSYMBOL_Decl = 44,                      /* Decl  */
  YYSYMBOL_ConstDecl = 45,                 /* ConstDecl  */
  YYSYMBOL_BType = 46,                     /* BType  */
  YYSYMBOL_ConstDef = 47,                  /* ConstDef  */
  YYSYMBOL_ConstInitVal = 48,              /* ConstInitVal  */
  YYSYMBOL_VarDecl = 49,                   /* VarDecl  */
  YYSYMBOL_VarDef = 50,                    /* VarDef  */
  YYSYMBOL_InitVal = 51,                   /* InitVal  */
  YYSYMBOL_FuncDef = 52,                   /* FuncDef  */
  YYSYMBOL_FuncFParams = 53,               /* FuncFParams  */
  YYSYMBOL_FuncFParam = 54,                /* FuncFParam  */
  YYSYMBOL_Block = 55,                     /* Block  */
  YYSYMBOL_BlockItem = 56,                 /* BlockItem  */
  YYSYMBOL_Stmt = 57,                      /* Stmt  */
  YYSYMBOL_Exp = 58,                       /* Exp  */
  YYSYMBOL_Cond = 59,                      /* Cond  */
  YYSYMBOL_LVal = 60,                      /* LVal  */
  YYSYMBOL_PrimaryExp = 61,                /* PrimaryExp  */
  YYSYMBOL_Number = 62,                    /* Number  */
  YYSYMBOL_UnaryExp = 63,                  /* UnaryExp  */
  YYSYMBOL_UnaryOp = 64,                   /* UnaryOp  */
  YYSYMBOL_FuncRParams = 65,               /* FuncRParams  */
  YYSYMBOL_MulExp = 66,                    /* MulExp  */
  YYSYMBOL_AddExp = 67,                    /* AddExp  */
  YYSYMBOL_RelExp = 68,                    /* RelExp  */
  YYSYMBOL_EqExp = 69,                     /* EqExp  */
  YYSYMBOL_LAndExp = 70,                   /* LAndExp  */
  YYSYMBOL_LOrExp = 71,                    /* LOrExp  */
  YYSYMBOL_ConstExp = 72,                  /* ConstExp  */
  YYSYMBOL_Decl_or_FuncDef = 73,           /* Decl_or_FuncDef  */
  YYSYMBOL_MulOp = 74,                     /* MulOp  */
  YYSYMBOL_AddOp = 75,                     /* AddOp  */
  YYSYMBOL_RelOp = 76,                     /* RelOp  */
  YYSYMBOL_EqOp = 77,                      /* EqOp  */
  YYSYMBOL_COMMA_ConstDef_Group = 78,      /* COMMA_ConstDef_Group  */
  YYSYMBOL_LBRACKET_ConstExp_RBRACKET_Group = 79, /* LBRACKET_ConstExp_RBRACKET_Group  */
  YYSYMBOL_COMMA_ConstInitVal_Group = 80,  /* COMMA_ConstInitVal_Group  */
  YYSYMBOL_COMMA_VarDef_Group = 81,        /* COMMA_VarDef_Group  */
  YYSYMBOL_COMMA_InitVal_Group = 82,       /* COMMA_InitVal_Group  */
  YYSYMBOL_COMMA_FuncFParam_Group = 83,    /* COMMA_FuncFParam_Group  */
  YYSYMBOL_LBRACKET_Exp_RBRACKET_Group = 84, /* LBRACKET_Exp_RBRACKET_Group  */
  YYSYMBOL_BlockItem_Group = 85,           /* BlockItem_Group  */
  YYSYMBOL_COMMA_Exp_Group = 86,           /* COMMA_Exp_Group  */
  YYSYMBOL_CompUnit_Optional = 87,         /* CompUnit_Optional  */
  YYSYMBOL_ConstInitVal_COMMA_ConstInitVal_Group_Optional = 88, /* ConstInitVal_COMMA_ConstInitVal_Group_Optional  */
  YYSYMBOL_InitVal_COMMA_InitVal_Group_Optional = 89, /* InitVal_COMMA_InitVal_Group_Optional  */
  YYSYMBOL_FuncFParams_Optional = 90,      /* FuncFParams_Optional  */
  YYSYMBOL_LBRACKET_RBRACKET_LBRACKET_Exp_RBRACKET_Group_Optional = 91, /* LBRACKET_RBRACKET_LBRACKET_Exp_RBRACKET_Group_Optional  */
  YYSYMBOL_Exp_Optional = 92,              /* Exp_Optional  */
  YYSYMBOL_FuncRParams_Optional = 93,      /* FuncRParams_Optional  */
  YYSYMBOL_ELSE_Stmt_Optional = 94         /* ELSE_Stmt_Optional  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_uint8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   184

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  42
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  53
/* YYNRULES -- Number of rules.  */
#define YYNRULES  106
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  176

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   296


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int8 yyrline[] =
{
       0,    56,    56,    57,    57,    58,    59,    59,    59,    60,
      61,    61,    62,    63,    63,    64,    64,    65,    66,    67,
      68,    69,    69,    70,    70,    70,    70,    70,    70,    70,
      70,    70,    71,    72,    73,    74,    74,    74,    75,    75,
      76,    76,    76,    77,    77,    77,    78,    79,    79,    80,
      80,    81,    81,    82,    82,    83,    83,    84,    84,    85,
      86,    86,    87,    87,    87,    88,    88,    89,    89,    89,
      89,    90,    90,    91,    91,    92,    92,    93,    93,    94,
      94,    95,    95,    96,    96,    97,    97,    98,    98,    99,
      99,   100,   100,   101,   101,   102,   102,   103,   103,   104,
     104,   105,   105,   106,   106,   107,   107
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "INT", "FLOAT", "VOID",
  "CONST", "IF", "ELSE", "WHILE", "BREAK", "CONTINUE", "RETURN", "FOR",
  "Ident", "IntConst", "floatConst", "ADD", "SUB", "MUL", "DIV", "MOD",
  "LPAREN", "RPAREN", "LBRACKET", "RBRACKET", "LBRACE", "RBRACE", "LESS",
  "LESS_EQUAL", "GREATER", "GREATER_EQUAL", "EQUAL", "NOT_EQUAL", "AND",
  "OR", "NOT", "ASSIGN", "ADD_ASSIGN", "COMMA", "SEMICOLON", "ERROR",
  "$accept", "CompUnit", "Decl", "ConstDecl", "BType", "ConstDef",
  "ConstInitVal", "VarDecl", "VarDef", "InitVal", "FuncDef", "FuncFParams",
  "FuncFParam", "Block", "BlockItem", "Stmt", "Exp", "Cond", "LVal",
  "PrimaryExp", "Number", "UnaryExp", "UnaryOp", "FuncRParams", "MulExp",
  "AddExp", "RelExp", "EqExp", "LAndExp", "LOrExp", "ConstExp",
  "Decl_or_FuncDef", "MulOp", "AddOp", "RelOp", "EqOp",
  "COMMA_ConstDef_Group", "LBRACKET_ConstExp_RBRACKET_Group",
  "COMMA_ConstInitVal_Group", "COMMA_VarDef_Group", "COMMA_InitVal_Group",
  "COMMA_FuncFParam_Group", "LBRACKET_Exp_RBRACKET_Group",
  "BlockItem_Group", "COMMA_Exp_Group", "CompUnit_Optional",
  "ConstInitVal_COMMA_ConstInitVal_Group_Optional",
  "InitVal_COMMA_InitVal_Group_Optional", "FuncFParams_Optional",
  "LBRACKET_RBRACKET_LBRACKET_Exp_RBRACKET_Group_Optional", "Exp_Optional",
  "FuncRParams_Optional", "ELSE_Stmt_Optional", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-117)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    -117,    10,    14,  -117,  -117,  -117,  -117,    26,  -117,  -117,
       1,  -117,  -117,  -117,     9,    41,  -117,  -117,  -117,    26,
     -16,    11,   -15,    16,    50,  -117,  -117,    45,   135,    63,
      56,  -117,   122,     9,  -117,    47,    43,    58,    65,  -117,
    -117,  -117,  -117,   135,  -117,  -117,  -117,  -117,  -117,   135,
      54,    -5,    70,    63,  -117,  -117,    -5,  -117,  -117,   122,
    -117,  -117,  -117,    72,  -117,    26,  -117,  -117,   135,    77,
      79,  -117,  -117,  -117,  -117,   135,  -117,  -117,   135,  -117,
    -117,    88,  -117,    89,  -117,  -117,    31,  -117,  -117,    94,
     135,  -117,  -117,    54,    80,  -117,    82,  -117,    77,   104,
     105,    90,    91,   135,   107,  -117,  -117,    56,  -117,  -117,
    -117,  -117,    98,    93,   103,  -117,   120,    63,   122,   135,
     135,  -117,  -117,   106,    26,   135,  -117,   135,  -117,  -117,
    -117,   124,    -5,    -3,    27,   121,   119,   133,  -117,   145,
     123,  -117,    96,  -117,  -117,  -117,  -117,   135,  -117,  -117,
     135,   135,   135,    96,  -117,   125,  -117,   152,    -5,    -3,
      27,   121,  -117,   135,    96,  -117,   126,  -117,   135,   127,
     145,   130,   135,   138,    96,  -117
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
      92,    91,     0,     1,     6,     7,     8,     0,    60,     3,
       0,     4,    61,     2,     0,    76,    80,    76,    74,    98,
      13,     0,     0,     0,     0,    97,    84,     0,     0,     0,
       0,    12,     0,     0,     5,   100,    18,     0,    86,    38,
      39,    43,    44,     0,    45,    36,    40,    37,    47,     0,
      49,    59,     0,    96,    14,    15,    32,    76,    79,    94,
       9,    10,    73,     0,    19,     0,    88,    17,   104,    34,
       0,    42,    62,    63,    64,     0,    65,    66,     0,    75,
      82,     0,    78,     0,    86,    83,   102,    90,   103,     0,
       0,    35,    48,    50,    95,    16,    93,    11,    99,     0,
       0,     0,     0,   102,     0,    20,    21,     0,    25,    87,
      22,   101,    36,     0,    46,    41,     0,     0,     0,     0,
       0,    28,    29,     0,     0,     0,    24,     0,    85,    81,
      77,     0,    51,    53,    55,    57,    33,     0,    30,     0,
       0,    89,   102,    67,    69,    68,    70,     0,    71,    72,
       0,     0,     0,   102,    86,     0,    23,   106,    52,    54,
      56,    58,    27,     0,   102,    26,     0,   105,     0,     0,
       0,     0,     0,     0,   102,    31
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -117,  -117,    78,  -117,     4,   132,   -56,  -117,   139,   -51,
    -117,  -117,   108,   137,  -117,   -88,   -29,  -116,   -70,  -117,
    -117,   -42,  -117,  -117,    92,   -27,    22,    24,    25,  -117,
     148,  -117,  -117,  -117,  -117,  -117,  -117,   161,  -117,  -117,
    -117,  -117,    95,  -117,  -117,  -117,  -117,  -117,  -117,  -117,
      81,  -117,  -117
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,     1,     8,     9,    24,    18,    60,    11,    16,    54,
      12,    25,    26,   108,   109,   110,   111,   131,    45,    46,
      47,    48,    49,    88,    50,    56,   133,   134,   135,   136,
      61,    13,    75,    78,   147,   150,    23,    20,    96,    21,
      94,    36,    69,    86,   114,     2,    83,    81,    27,    64,
     113,    89,   165
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      55,    51,    80,    82,   137,    51,    10,    71,    28,    28,
       3,    14,    76,    77,    70,    15,   112,     4,     5,     6,
       7,    29,    32,    17,    55,   143,   144,   145,   146,     4,
       5,     6,    51,    92,     4,     5,     6,     7,    99,    87,
     100,   101,   102,   103,   104,    38,    39,    40,    41,    42,
      30,    31,   169,    43,   157,    33,    34,    66,   105,   148,
     149,   116,   130,    19,    35,   162,   129,    44,    37,   155,
      57,    63,   112,    72,    73,    74,   167,    38,    39,    40,
      41,    42,    65,   112,    66,    43,   175,    68,    55,    53,
     107,    51,   132,   132,   112,    79,   140,    84,   141,    44,
     171,    90,    91,    99,   112,   100,   101,   102,   103,   104,
      38,    39,    40,    41,    42,    95,    97,   115,    43,   117,
     158,   118,    66,   132,   132,   132,   119,   120,   139,   124,
     121,   122,    44,   126,   166,   125,    38,    39,    40,    41,
      42,   132,   127,   173,    43,   128,   138,   142,    59,    38,
      39,    40,    41,    42,   152,   151,   153,    43,    44,   154,
     164,   174,   163,   156,   106,    62,   168,   170,   172,    58,
      93,    44,   159,    85,    67,   160,    52,   161,    22,    98,
       0,     0,     0,     0,   123
};

static const yytype_int16 yycheck[] =
{
      29,    28,    53,    59,   120,    32,     2,    49,    24,    24,
       0,     7,    17,    18,    43,    14,    86,     3,     4,     5,
       6,    37,    37,    14,    53,    28,    29,    30,    31,     3,
       4,     5,    59,    75,     3,     4,     5,     6,     7,    68,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      39,    40,   168,    22,   142,    39,    40,    26,    27,    32,
      33,    90,   118,    22,    14,   153,   117,    36,    23,   139,
      14,    24,   142,    19,    20,    21,   164,    14,    15,    16,
      17,    18,    39,   153,    26,    22,   174,    22,   117,    26,
      86,   118,   119,   120,   164,    25,   125,    25,   127,    36,
     170,    24,    23,     7,   174,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    27,    27,    23,    22,    39,
     147,    39,    26,   150,   151,   152,    22,    22,   124,    22,
      40,    40,    36,    40,   163,    37,    14,    15,    16,    17,
      18,   168,    39,   172,    22,    25,    40,    23,    26,    14,
      15,    16,    17,    18,    35,    34,    23,    22,    36,    14,
       8,    23,    37,    40,    86,    33,    40,    40,    38,    30,
      78,    36,   150,    65,    37,   151,    28,   152,    17,    84,
      -1,    -1,    -1,    -1,   103
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,    43,    87,     0,     3,     4,     5,     6,    44,    45,
      46,    49,    52,    73,    46,    14,    50,    14,    47,    22,
      79,    81,    79,    78,    46,    53,    54,    90,    24,    37,
      39,    40,    37,    39,    40,    14,    83,    23,    14,    15,
      16,    17,    18,    22,    36,    60,    61,    62,    63,    64,
      66,    67,    72,    26,    51,    58,    67,    14,    50,    26,
      48,    72,    47,    24,    91,    39,    26,    55,    22,    84,
      58,    63,    19,    20,    21,    74,    17,    18,    75,    25,
      51,    89,    48,    88,    25,    54,    85,    58,    65,    93,
      24,    23,    63,    66,    82,    27,    80,    27,    84,     7,
       9,    10,    11,    12,    13,    27,    44,    46,    55,    56,
      57,    58,    60,    92,    86,    23,    58,    39,    39,    22,
      22,    40,    40,    92,    22,    37,    40,    39,    25,    51,
      48,    59,    67,    68,    69,    70,    71,    59,    40,    46,
      58,    58,    23,    28,    29,    30,    31,    76,    32,    33,
      77,    34,    35,    23,    14,    60,    40,    57,    67,    68,
      69,    70,    57,    37,     8,    94,    58,    57,    40,    59,
      40,    60,    38,    58,    23,    57
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    42,    43,    44,    44,    45,    46,    46,    46,    47,
      48,    48,    49,    50,    50,    51,    51,    52,    53,    54,
      55,    56,    56,    57,    57,    57,    57,    57,    57,    57,
      57,    57,    58,    59,    60,    61,    61,    61,    62,    62,
      63,    63,    63,    64,    64,    64,    65,    66,    66,    67,
      67,    68,    68,    69,    69,    70,    70,    71,    71,    72,
      73,    73,    74,    74,    74,    75,    75,    76,    76,    76,
      76,    77,    77,    78,    78,    79,    79,    80,    80,    81,
      81,    82,    82,    83,    83,    84,    84,    85,    85,    86,
      86,    87,    87,    88,    88,    89,    89,    90,    90,    91,
      91,    92,    92,    93,    93,    94,    94
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     2,     1,     1,     5,     1,     1,     1,     4,
       1,     3,     4,     2,     4,     1,     3,     6,     2,     3,
       3,     1,     1,     4,     2,     1,     6,     5,     2,     2,
       3,    14,     1,     1,     2,     3,     1,     1,     1,     1,
       1,     4,     2,     1,     1,     1,     2,     1,     3,     1,
       3,     1,     3,     1,     3,     1,     3,     1,     3,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     3,     0,     4,     0,     3,     0,     3,
       0,     3,     0,     3,     0,     4,     0,     2,     0,     3,
       0,     1,     0,     2,     0,     2,     0,     1,     0,     3,
       0,     1,     0,     1,     0,     2,     0
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* CompUnit: CompUnit_Optional Decl_or_FuncDef  */
#line 56 "parser.y"
                                                    { (yyval.node) = new_node("CompUnit", {(yyvsp[-1].node), (yyvsp[0].node)}); syntax_tree.root = (yyval.node); }
#line 1314 "parser.cc"
    break;

  case 3: /* Decl: ConstDecl  */
#line 57 "parser.y"
                 { (yyval.node) = new_node("Decl", {(yyvsp[0].node)}); }
#line 1320 "parser.cc"
    break;

  case 4: /* Decl: VarDecl  */
#line 57 "parser.y"
                                                             { (yyval.node) = new_node("Decl", {(yyvsp[0].node)}); }
#line 1326 "parser.cc"
    break;

  case 5: /* ConstDecl: CONST BType ConstDef COMMA_ConstDef_Group SEMICOLON  */
#line 58 "parser.y"
                                                                 { (yyval.node) = new_node("ConstDecl", {(yyvsp[-4].node), (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node)}); }
#line 1332 "parser.cc"
    break;

  case 6: /* BType: INT  */
#line 59 "parser.y"
            { (yyval.node) = new_node("BType", {(yyvsp[0].node)}); }
#line 1338 "parser.cc"
    break;

  case 7: /* BType: FLOAT  */
#line 59 "parser.y"
                                                      { (yyval.node) = new_node("BType", {(yyvsp[0].node)}); }
#line 1344 "parser.cc"
    break;

  case 8: /* BType: VOID  */
#line 59 "parser.y"
                                                                                                { (yyval.node) = new_node("BType", {(yyvsp[0].node)}); }
#line 1350 "parser.cc"
    break;

  case 9: /* ConstDef: Ident LBRACKET_ConstExp_RBRACKET_Group ASSIGN ConstInitVal  */
#line 60 "parser.y"
                                                                       { (yyval.node) = new_node("ConstDef", {(yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node)}); }
#line 1356 "parser.cc"
    break;

  case 10: /* ConstInitVal: ConstExp  */
#line 61 "parser.y"
                        { (yyval.node) = new_node("ConstInitVal", {(yyvsp[0].node)}); }
#line 1362 "parser.cc"
    break;

  case 11: /* ConstInitVal: LBRACE ConstInitVal_COMMA_ConstInitVal_Group_Optional RBRACE  */
#line 61 "parser.y"
                                                                                                                                 { (yyval.node) = new_node("ConstInitVal", {(yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node)}); }
#line 1368 "parser.cc"
    break;

  case 12: /* VarDecl: BType VarDef COMMA_VarDef_Group SEMICOLON  */
#line 62 "parser.y"
                                                     { (yyval.node) = new_node("VarDecl", {(yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node)}); }
#line 1374 "parser.cc"
    break;

  case 13: /* VarDef: Ident LBRACKET_ConstExp_RBRACKET_Group  */
#line 63 "parser.y"
                                                { (yyval.node) = new_node("VarDef", {(yyvsp[-1].node), (yyvsp[0].node)}); }
#line 1380 "parser.cc"
    break;

  case 14: /* VarDef: Ident LBRACKET_ConstExp_RBRACKET_Group ASSIGN InitVal  */
#line 63 "parser.y"
                                                                                                                                                { (yyval.node) = new_node("VarDef", {(yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node)}); }
#line 1386 "parser.cc"
    break;

  case 15: /* InitVal: Exp  */
#line 64 "parser.y"
              { (yyval.node) = new_node("InitVal", {(yyvsp[0].node)}); }
#line 1392 "parser.cc"
    break;

  case 16: /* InitVal: LBRACE InitVal_COMMA_InitVal_Group_Optional RBRACE  */
#line 64 "parser.y"
                                                                                                        { (yyval.node) = new_node("InitVal", {(yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node)}); }
#line 1398 "parser.cc"
    break;

  case 17: /* FuncDef: BType Ident LPAREN FuncFParams_Optional RPAREN Block  */
#line 65 "parser.y"
                                                                { (yyval.node) = new_node("FuncDef", {(yyvsp[-5].node), (yyvsp[-4].node), (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node)}); }
#line 1404 "parser.cc"
    break;

  case 18: /* FuncFParams: FuncFParam COMMA_FuncFParam_Group  */
#line 66 "parser.y"
                                                 { (yyval.node) = new_node("FuncFParams", {(yyvsp[-1].node), (yyvsp[0].node)}); }
#line 1410 "parser.cc"
    break;

  case 19: /* FuncFParam: BType Ident LBRACKET_RBRACKET_LBRACKET_Exp_RBRACKET_Group_Optional  */
#line 67 "parser.y"
                                                                                 { (yyval.node) = new_node("FuncFParam", {(yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node)}); }
#line 1416 "parser.cc"
    break;

  case 20: /* Block: LBRACE BlockItem_Group RBRACE  */
#line 68 "parser.y"
                                       { (yyval.node) = new_node("Block", {(yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node)}); }
#line 1422 "parser.cc"
    break;

  case 21: /* BlockItem: Decl  */
#line 69 "parser.y"
                 { (yyval.node) = new_node("BlockItem", {(yyvsp[0].node)}); }
#line 1428 "parser.cc"
    break;

  case 22: /* BlockItem: Stmt  */
#line 69 "parser.y"
                                                               { (yyval.node) = new_node("BlockItem", {(yyvsp[0].node)}); }
#line 1434 "parser.cc"
    break;

  case 23: /* Stmt: LVal ASSIGN Exp SEMICOLON  */
#line 70 "parser.y"
                                 { (yyval.node) = new_node("Stmt", {(yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node)}); }
#line 1440 "parser.cc"
    break;

  case 24: /* Stmt: Exp_Optional SEMICOLON  */
#line 70 "parser.y"
                                                                                                       { (yyval.node) = new_node("Stmt", {(yyvsp[-1].node), (yyvsp[0].node)}); }
#line 1446 "parser.cc"
    break;

  case 25: /* Stmt: Block  */
#line 70 "parser.y"
                                                                                                                                                    { (yyval.node) = new_node("Stmt", {(yyvsp[0].node)}); }
#line 1452 "parser.cc"
    break;

  case 26: /* Stmt: IF LPAREN Cond RPAREN Stmt ELSE_Stmt_Optional  */
#line 70 "parser.y"
                                                                                                                                                                                                                                     { (yyval.node) = new_node("Stmt", {(yyvsp[-5].node), (yyvsp[-4].node), (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node)}); }
#line 1458 "parser.cc"
    break;

  case 27: /* Stmt: WHILE LPAREN Cond RPAREN Stmt  */
#line 70 "parser.y"
                                                                                                                                                                                                                                                                                                                          { (yyval.node) = new_node("Stmt", {(yyvsp[-4].node), (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node)}); }
#line 1464 "parser.cc"
    break;

  case 28: /* Stmt: BREAK SEMICOLON  */
#line 70 "parser.y"
                                                                                                                                                                                                                                                                                                                                                                                             { (yyval.node) = new_node("Stmt", {(yyvsp[-1].node), (yyvsp[0].node)}); }
#line 1470 "parser.cc"
    break;

  case 29: /* Stmt: CONTINUE SEMICOLON  */
#line 70 "parser.y"
                                                                                                                                                                                                                                                                                                                                                                                                                                                       { (yyval.node) = new_node("Stmt", {(yyvsp[-1].node), (yyvsp[0].node)}); }
#line 1476 "parser.cc"
    break;

  case 30: /* Stmt: RETURN Exp_Optional SEMICOLON  */
#line 70 "parser.y"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            { (yyval.node) = new_node("Stmt", {(yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node)}); }
#line 1482 "parser.cc"
    break;

  case 31: /* Stmt: FOR LPAREN BType LVal ASSIGN Exp SEMICOLON Cond SEMICOLON LVal ADD_ASSIGN Exp RPAREN Stmt  */
#line 70 "parser.y"
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 { (yyval.node) = new_node("Stmt", {(yyvsp[-13].node), (yyvsp[-12].node), (yyvsp[-11].node), (yyvsp[-10].node), (yyvsp[-9].node), (yyvsp[-8].node), (yyvsp[-7].node), (yyvsp[-6].node), (yyvsp[-5].node), (yyvsp[-4].node), (yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node)}); }
#line 1488 "parser.cc"
    break;

  case 32: /* Exp: AddExp  */
#line 71 "parser.y"
              { (yyval.node) = new_node("Exp", {(yyvsp[0].node)}); }
#line 1494 "parser.cc"
    break;

  case 33: /* Cond: LOrExp  */
#line 72 "parser.y"
               { (yyval.node) = new_node("Cond", {(yyvsp[0].node)}); }
#line 1500 "parser.cc"
    break;

  case 34: /* LVal: Ident LBRACKET_Exp_RBRACKET_Group  */
#line 73 "parser.y"
                                          { (yyval.node) = new_node("LVal", {(yyvsp[-1].node), (yyvsp[0].node)}); }
#line 1506 "parser.cc"
    break;

  case 35: /* PrimaryExp: LPAREN Exp RPAREN  */
#line 74 "parser.y"
                               { (yyval.node) = new_node("PrimaryExp", {(yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node)}); }
#line 1512 "parser.cc"
    break;

  case 36: /* PrimaryExp: LVal  */
#line 74 "parser.y"
                                                                                     { (yyval.node) = new_node("PrimaryExp", {(yyvsp[0].node)}); }
#line 1518 "parser.cc"
    break;

  case 37: /* PrimaryExp: Number  */
#line 74 "parser.y"
                                                                                                                                      { (yyval.node) = new_node("PrimaryExp", {(yyvsp[0].node)}); }
#line 1524 "parser.cc"
    break;

  case 38: /* Number: IntConst  */
#line 75 "parser.y"
                  { (yyval.node) = new_node("Number", {(yyvsp[0].node)}); }
#line 1530 "parser.cc"
    break;

  case 39: /* Number: floatConst  */
#line 75 "parser.y"
                                                                   { (yyval.node) = new_node("Number", {(yyvsp[0].node)}); }
#line 1536 "parser.cc"
    break;

  case 40: /* UnaryExp: PrimaryExp  */
#line 76 "parser.y"
                      { (yyval.node) = new_node("UnaryExp", {(yyvsp[0].node)}); }
#line 1542 "parser.cc"
    break;

  case 41: /* UnaryExp: Ident LPAREN FuncRParams_Optional RPAREN  */
#line 76 "parser.y"
                                                                                                      { (yyval.node) = new_node("UnaryExp", {(yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node)}); }
#line 1548 "parser.cc"
    break;

  case 42: /* UnaryExp: UnaryOp UnaryExp  */
#line 76 "parser.y"
                                                                                                                                                                           { (yyval.node) = new_node("UnaryExp", {(yyvsp[-1].node), (yyvsp[0].node)}); }
#line 1554 "parser.cc"
    break;

  case 43: /* UnaryOp: ADD  */
#line 77 "parser.y"
              { (yyval.node) = new_node("UnaryOp", {(yyvsp[0].node)}); }
#line 1560 "parser.cc"
    break;

  case 44: /* UnaryOp: SUB  */
#line 77 "parser.y"
                                                        { (yyval.node) = new_node("UnaryOp", {(yyvsp[0].node)}); }
#line 1566 "parser.cc"
    break;

  case 45: /* UnaryOp: NOT  */
#line 77 "parser.y"
                                                                                                   { (yyval.node) = new_node("UnaryOp", {(yyvsp[0].node)}); }
#line 1572 "parser.cc"
    break;

  case 46: /* FuncRParams: Exp COMMA_Exp_Group  */
#line 78 "parser.y"
                                   { (yyval.node) = new_node("FuncRParams", {(yyvsp[-1].node), (yyvsp[0].node)}); }
#line 1578 "parser.cc"
    break;

  case 47: /* MulExp: UnaryExp  */
#line 79 "parser.y"
                  { (yyval.node) = new_node("MulExp", {(yyvsp[0].node)}); }
#line 1584 "parser.cc"
    break;

  case 48: /* MulExp: MulExp MulOp UnaryExp  */
#line 79 "parser.y"
                                                                              { (yyval.node) = new_node("MulExp", {(yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node)}); }
#line 1590 "parser.cc"
    break;

  case 49: /* AddExp: MulExp  */
#line 80 "parser.y"
                { (yyval.node) = new_node("AddExp", {(yyvsp[0].node)}); }
#line 1596 "parser.cc"
    break;

  case 50: /* AddExp: AddExp AddOp MulExp  */
#line 80 "parser.y"
                                                                          { (yyval.node) = new_node("AddExp", {(yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node)}); }
#line 1602 "parser.cc"
    break;

  case 51: /* RelExp: AddExp  */
#line 81 "parser.y"
                { (yyval.node) = new_node("RelExp", {(yyvsp[0].node)}); }
#line 1608 "parser.cc"
    break;

  case 52: /* RelExp: RelExp RelOp AddExp  */
#line 81 "parser.y"
                                                                          { (yyval.node) = new_node("RelExp", {(yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node)}); }
#line 1614 "parser.cc"
    break;

  case 53: /* EqExp: RelExp  */
#line 82 "parser.y"
               { (yyval.node) = new_node("EqExp", {(yyvsp[0].node)}); }
#line 1620 "parser.cc"
    break;

  case 54: /* EqExp: EqExp EqOp RelExp  */
#line 82 "parser.y"
                                                                      { (yyval.node) = new_node("EqExp", {(yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node)}); }
#line 1626 "parser.cc"
    break;

  case 55: /* LAndExp: EqExp  */
#line 83 "parser.y"
                { (yyval.node) = new_node("LAndExp", {(yyvsp[0].node)}); }
#line 1632 "parser.cc"
    break;

  case 56: /* LAndExp: LAndExp AND EqExp  */
#line 83 "parser.y"
                                                                         { (yyval.node) = new_node("LAndExp", {(yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node)}); }
#line 1638 "parser.cc"
    break;

  case 57: /* LOrExp: LAndExp  */
#line 84 "parser.y"
                 { (yyval.node) = new_node("LOrExp", {(yyvsp[0].node)}); }
#line 1644 "parser.cc"
    break;

  case 58: /* LOrExp: LOrExp OR LAndExp  */
#line 84 "parser.y"
                                                                         { (yyval.node) = new_node("LOrExp", {(yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node)}); }
#line 1650 "parser.cc"
    break;

  case 59: /* ConstExp: AddExp  */
#line 85 "parser.y"
                   { (yyval.node) = new_node("ConstExp", {(yyvsp[0].node)}); }
#line 1656 "parser.cc"
    break;

  case 60: /* Decl_or_FuncDef: Decl  */
#line 86 "parser.y"
                       { (yyval.node) = new_node("Decl_or_FuncDef", {(yyvsp[0].node)}); }
#line 1662 "parser.cc"
    break;

  case 61: /* Decl_or_FuncDef: FuncDef  */
#line 86 "parser.y"
                                                                              { (yyval.node) = new_node("Decl_or_FuncDef", {(yyvsp[0].node)}); }
#line 1668 "parser.cc"
    break;

  case 62: /* MulOp: MUL  */
#line 87 "parser.y"
            { (yyval.node) = new_node("MulOp", {(yyvsp[0].node)}); }
#line 1674 "parser.cc"
    break;

  case 63: /* MulOp: DIV  */
#line 87 "parser.y"
                                                    { (yyval.node) = new_node("MulOp", {(yyvsp[0].node)}); }
#line 1680 "parser.cc"
    break;

  case 64: /* MulOp: MOD  */
#line 87 "parser.y"
                                                                                             { (yyval.node) = new_node("MulOp", {(yyvsp[0].node)}); }
#line 1686 "parser.cc"
    break;

  case 65: /* AddOp: ADD  */
#line 88 "parser.y"
            { (yyval.node) = new_node("AddOp", {(yyvsp[0].node)}); }
#line 1692 "parser.cc"
    break;

  case 66: /* AddOp: SUB  */
#line 88 "parser.y"
                                                     { (yyval.node) = new_node("AddOp", {(yyvsp[0].node)}); }
#line 1698 "parser.cc"
    break;

  case 67: /* RelOp: LESS  */
#line 89 "parser.y"
             { (yyval.node) = new_node("RelOp", {(yyvsp[0].node)}); }
#line 1704 "parser.cc"
    break;

  case 68: /* RelOp: GREATER  */
#line 89 "parser.y"
                                                         { (yyval.node) = new_node("RelOp", {(yyvsp[0].node)}); }
#line 1710 "parser.cc"
    break;

  case 69: /* RelOp: LESS_EQUAL  */
#line 89 "parser.y"
                                                                                                        { (yyval.node) = new_node("RelOp", {(yyvsp[0].node)}); }
#line 1716 "parser.cc"
    break;

  case 70: /* RelOp: GREATER_EQUAL  */
#line 89 "parser.y"
                                                                                                                                                           { (yyval.node) = new_node("RelOp", {(yyvsp[0].node)}); }
#line 1722 "parser.cc"
    break;

  case 71: /* EqOp: EQUAL  */
#line 90 "parser.y"
             { (yyval.node) = new_node("EqOp", {(yyvsp[0].node)}); }
#line 1728 "parser.cc"
    break;

  case 72: /* EqOp: NOT_EQUAL  */
#line 90 "parser.y"
                                                           { (yyval.node) = new_node("EqOp", {(yyvsp[0].node)}); }
#line 1734 "parser.cc"
    break;

  case 73: /* COMMA_ConstDef_Group: COMMA_ConstDef_Group COMMA ConstDef  */
#line 91 "parser.y"
                                                           { (yyval.node) = new_node("COMMA_ConstDef_Group", {(yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node)}); }
#line 1740 "parser.cc"
    break;

  case 74: /* COMMA_ConstDef_Group: %empty  */
#line 91 "parser.y"
                                                                                                                       { (yyval.node) = new_node("COMMA_ConstDef_Group", {}); }
#line 1746 "parser.cc"
    break;

  case 75: /* LBRACKET_ConstExp_RBRACKET_Group: LBRACKET_ConstExp_RBRACKET_Group LBRACKET ConstExp RBRACKET  */
#line 92 "parser.y"
                                                                                               { (yyval.node) = new_node("LBRACKET_ConstExp_RBRACKET_Group", {(yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node)}); }
#line 1752 "parser.cc"
    break;

  case 76: /* LBRACKET_ConstExp_RBRACKET_Group: %empty  */
#line 92 "parser.y"
                                                                                                                                                                           { (yyval.node) = new_node("LBRACKET_ConstExp_RBRACKET_Group", {}); }
#line 1758 "parser.cc"
    break;

  case 77: /* COMMA_ConstInitVal_Group: COMMA_ConstInitVal_Group COMMA ConstInitVal  */
#line 93 "parser.y"
                                                                       { (yyval.node) = new_node("COMMA_ConstInitVal_Group", {(yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node)}); }
#line 1764 "parser.cc"
    break;

  case 78: /* COMMA_ConstInitVal_Group: %empty  */
#line 93 "parser.y"
                                                                                                                                       { (yyval.node) = new_node("COMMA_ConstInitVal_Group", {}); }
#line 1770 "parser.cc"
    break;

  case 79: /* COMMA_VarDef_Group: COMMA_VarDef_Group COMMA VarDef  */
#line 94 "parser.y"
                                                     { (yyval.node) = new_node("COMMA_VarDef_Group", {(yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node)}); }
#line 1776 "parser.cc"
    break;

  case 80: /* COMMA_VarDef_Group: %empty  */
#line 94 "parser.y"
                                                                                                               { (yyval.node) = new_node("COMMA_VarDef_Group", {}); }
#line 1782 "parser.cc"
    break;

  case 81: /* COMMA_InitVal_Group: COMMA_InitVal_Group COMMA InitVal  */
#line 95 "parser.y"
                                                        { (yyval.node) = new_node("COMMA_InitVal_Group", {(yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node)}); }
#line 1788 "parser.cc"
    break;

  case 82: /* COMMA_InitVal_Group: %empty  */
#line 95 "parser.y"
                                                                                                                   { (yyval.node) = new_node("COMMA_InitVal_Group", {}); }
#line 1794 "parser.cc"
    break;

  case 83: /* COMMA_FuncFParam_Group: COMMA_FuncFParam_Group COMMA FuncFParam  */
#line 96 "parser.y"
                                                                 { (yyval.node) = new_node("COMMA_FuncFParam_Group", {(yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node)}); }
#line 1800 "parser.cc"
    break;

  case 84: /* COMMA_FuncFParam_Group: %empty  */
#line 96 "parser.y"
                                                                                                                               { (yyval.node) = new_node("COMMA_FuncFParam_Group", {}); }
#line 1806 "parser.cc"
    break;

  case 85: /* LBRACKET_Exp_RBRACKET_Group: LBRACKET_Exp_RBRACKET_Group LBRACKET Exp RBRACKET  */
#line 97 "parser.y"
                                                                                { (yyval.node) = new_node("LBRACKET_Exp_RBRACKET_Group", {(yyvsp[-3].node), (yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node)}); }
#line 1812 "parser.cc"
    break;

  case 86: /* LBRACKET_Exp_RBRACKET_Group: %empty  */
#line 97 "parser.y"
                                                                                                                                                       { (yyval.node) = new_node("LBRACKET_Exp_RBRACKET_Group", {}); }
#line 1818 "parser.cc"
    break;

  case 87: /* BlockItem_Group: BlockItem_Group BlockItem  */
#line 98 "parser.y"
                                            { (yyval.node) = new_node("BlockItem_Group", {(yyvsp[-1].node), (yyvsp[0].node)}); }
#line 1824 "parser.cc"
    break;

  case 88: /* BlockItem_Group: %empty  */
#line 98 "parser.y"
                                                                                               { (yyval.node) = new_node("BlockItem_Group", {}); }
#line 1830 "parser.cc"
    break;

  case 89: /* COMMA_Exp_Group: COMMA_Exp_Group COMMA Exp  */
#line 99 "parser.y"
                                            { (yyval.node) = new_node("COMMA_Exp_Group", {(yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node)}); }
#line 1836 "parser.cc"
    break;

  case 90: /* COMMA_Exp_Group: %empty  */
#line 99 "parser.y"
                                                                                                   { (yyval.node) = new_node("COMMA_Exp_Group", {}); }
#line 1842 "parser.cc"
    break;

  case 91: /* CompUnit_Optional: CompUnit  */
#line 100 "parser.y"
                             { (yyval.node) = new_node("CompUnit_Optional", {(yyvsp[0].node)}); }
#line 1848 "parser.cc"
    break;

  case 92: /* CompUnit_Optional: %empty  */
#line 100 "parser.y"
                                                                              { (yyval.node) = new_node("CompUnit_Optional", {}); }
#line 1854 "parser.cc"
    break;

  case 93: /* ConstInitVal_COMMA_ConstInitVal_Group_Optional: ConstInitVal COMMA_ConstInitVal_Group  */
#line 101 "parser.y"
                                                                                       { (yyval.node) = new_node("ConstInitVal_COMMA_ConstInitVal_Group_Optional", {(yyvsp[-1].node), (yyvsp[0].node)}); }
#line 1860 "parser.cc"
    break;

  case 94: /* ConstInitVal_COMMA_ConstInitVal_Group_Optional: %empty  */
#line 101 "parser.y"
                                                                                                                                                                         { (yyval.node) = new_node("ConstInitVal_COMMA_ConstInitVal_Group_Optional", {}); }
#line 1866 "parser.cc"
    break;

  case 95: /* InitVal_COMMA_InitVal_Group_Optional: InitVal COMMA_InitVal_Group  */
#line 102 "parser.y"
                                                                   { (yyval.node) = new_node("InitVal_COMMA_InitVal_Group_Optional", {(yyvsp[-1].node), (yyvsp[0].node)}); }
#line 1872 "parser.cc"
    break;

  case 96: /* InitVal_COMMA_InitVal_Group_Optional: %empty  */
#line 102 "parser.y"
                                                                                                                                           { (yyval.node) = new_node("InitVal_COMMA_InitVal_Group_Optional", {}); }
#line 1878 "parser.cc"
    break;

  case 97: /* FuncFParams_Optional: FuncFParams  */
#line 103 "parser.y"
                                   { (yyval.node) = new_node("FuncFParams_Optional", {(yyvsp[0].node)}); }
#line 1884 "parser.cc"
    break;

  case 98: /* FuncFParams_Optional: %empty  */
#line 103 "parser.y"
                                                                                       { (yyval.node) = new_node("FuncFParams_Optional", {}); }
#line 1890 "parser.cc"
    break;

  case 99: /* LBRACKET_RBRACKET_LBRACKET_Exp_RBRACKET_Group_Optional: LBRACKET RBRACKET LBRACKET_Exp_RBRACKET_Group  */
#line 104 "parser.y"
                                                                                                       { (yyval.node) = new_node("LBRACKET_RBRACKET_LBRACKET_Exp_RBRACKET_Group_Optional", {(yyvsp[-2].node), (yyvsp[-1].node), (yyvsp[0].node)}); }
#line 1896 "parser.cc"
    break;

  case 100: /* LBRACKET_RBRACKET_LBRACKET_Exp_RBRACKET_Group_Optional: %empty  */
#line 104 "parser.y"
                                                                                                                                                                                                     { (yyval.node) = new_node("LBRACKET_RBRACKET_LBRACKET_Exp_RBRACKET_Group_Optional", {}); }
#line 1902 "parser.cc"
    break;

  case 101: /* Exp_Optional: Exp  */
#line 105 "parser.y"
                   { (yyval.node) = new_node("Exp_Optional", {(yyvsp[0].node)}); }
#line 1908 "parser.cc"
    break;

  case 102: /* Exp_Optional: %empty  */
#line 105 "parser.y"
                                                               { (yyval.node) = new_node("Exp_Optional", {}); }
#line 1914 "parser.cc"
    break;

  case 103: /* FuncRParams_Optional: FuncRParams  */
#line 106 "parser.y"
                                   { (yyval.node) = new_node("FuncRParams_Optional", {(yyvsp[0].node)}); }
#line 1920 "parser.cc"
    break;

  case 104: /* FuncRParams_Optional: %empty  */
#line 106 "parser.y"
                                                                                       { (yyval.node) = new_node("FuncRParams_Optional", {}); }
#line 1926 "parser.cc"
    break;

  case 105: /* ELSE_Stmt_Optional: ELSE Stmt  */
#line 107 "parser.y"
                               { (yyval.node) = new_node("ELSE_Stmt_Optional", {(yyvsp[-1].node), (yyvsp[0].node)}); }
#line 1932 "parser.cc"
    break;

  case 106: /* ELSE_Stmt_Optional: %empty  */
#line 107 "parser.y"
                                                                                     { (yyval.node) = new_node("ELSE_Stmt_Optional", {}); }
#line 1938 "parser.cc"
    break;


#line 1942 "parser.cc"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 108 "parser.y"

