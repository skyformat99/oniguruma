#include <ruby.h>
#include <oniguruma.h>
#include "rb_oniguruma.h"
#include "rb_oniguruma_oregexp.h"

void
Init_oniguruma()
{
  VALUE og_mOniguruma_Opt_Shortcuts;
  og_mOniguruma = rb_define_module("Oniguruma");
  
  Init_oniguruma_oregexp();
  Init_oniguruma_string_ext();
  Init_oniguruma_match_data_ext();
  
  /* Module Constants */
  rb_define_const(og_mOniguruma, "VERSION", 
    rb_str_new2(onig_version()));
  
  /* Encodings */
  rb_define_const(og_mOniguruma, "ENCODING_UNDEF",            INT2FIX(0));
  rb_define_const(og_mOniguruma, "ENCODING_ASCII",            INT2FIX(1));
  rb_define_const(og_mOniguruma, "ENCODING_ISO_8859_1",       INT2FIX(2));
  rb_define_const(og_mOniguruma, "ENCODING_ISO_8859_2",       INT2FIX(3));
  rb_define_const(og_mOniguruma, "ENCODING_ISO_8859_3",       INT2FIX(4));
  rb_define_const(og_mOniguruma, "ENCODING_ISO_8859_4",       INT2FIX(5));
  rb_define_const(og_mOniguruma, "ENCODING_ISO_8859_5",       INT2FIX(6));
  rb_define_const(og_mOniguruma, "ENCODING_ISO_8859_6",       INT2FIX(7));
  rb_define_const(og_mOniguruma, "ENCODING_ISO_8859_7",       INT2FIX(8));
  rb_define_const(og_mOniguruma, "ENCODING_ISO_8859_8",       INT2FIX(9));
  rb_define_const(og_mOniguruma, "ENCODING_ISO_8859_9",       INT2FIX(10));
  rb_define_const(og_mOniguruma, "ENCODING_ISO_8859_10",      INT2FIX(11));
  rb_define_const(og_mOniguruma, "ENCODING_ISO_8859_11",      INT2FIX(12));
  rb_define_const(og_mOniguruma, "ENCODING_ISO_8859_13",      INT2FIX(13));
  rb_define_const(og_mOniguruma, "ENCODING_ISO_8859_14",      INT2FIX(14));
  rb_define_const(og_mOniguruma, "ENCODING_ISO_8859_15",      INT2FIX(15));
  rb_define_const(og_mOniguruma, "ENCODING_ISO_8859_16",      INT2FIX(16));
  rb_define_const(og_mOniguruma, "ENCODING_UTF8",             INT2FIX(17));
  rb_define_const(og_mOniguruma, "ENCODING_EUC_JP",           INT2FIX(18));
  rb_define_const(og_mOniguruma, "ENCODING_EUC_TW",           INT2FIX(19));
  rb_define_const(og_mOniguruma, "ENCODING_EUC_KR",           INT2FIX(20));
  rb_define_const(og_mOniguruma, "ENCODING_EUC_CN",           INT2FIX(21));
  rb_define_const(og_mOniguruma, "ENCODING_SJIS",             INT2FIX(22));
  rb_define_const(og_mOniguruma, "ENCODING_KOI8",             INT2FIX(23));
  rb_define_const(og_mOniguruma, "ENCODING_KOI8_R",           INT2FIX(24));
  rb_define_const(og_mOniguruma, "ENCODING_BIG5",             INT2FIX(25));
  
  /* Syntaxes */
  rb_define_const(og_mOniguruma, "SYNTAX_DEFAULT",            INT2FIX(0));
  rb_define_const(og_mOniguruma, "SYNTAX_POSIX_BASIC",        INT2FIX(1));
  rb_define_const(og_mOniguruma, "SYNTAX_POSIX_EXTENDED",     INT2FIX(2));
  rb_define_const(og_mOniguruma, "SYNTAX_EMACS",              INT2FIX(3));
  rb_define_const(og_mOniguruma, "SYNTAX_GREP",               INT2FIX(4));
  rb_define_const(og_mOniguruma, "SYNTAX_GNU_REGEX",          INT2FIX(5));
  rb_define_const(og_mOniguruma, "SYNTAX_JAVA",               INT2FIX(6));
  rb_define_const(og_mOniguruma, "SYNTAX_PERL",               INT2FIX(7));
  rb_define_const(og_mOniguruma, "SYNTAX_RUBY",               INT2FIX(8));
  
  /* Options */                                                            /* Modifier */
  rb_define_const(og_mOniguruma, "OPTION_DEFAULT",            INT2FIX(0));
  rb_define_const(og_mOniguruma, "OPTION_NONE",               INT2FIX(0));
  rb_define_const(og_mOniguruma, "OPTION_IGNORECASE",         INT2FIX(1));    /* i */
  rb_define_const(og_mOniguruma, "OPTION_EXTEND",             INT2FIX(2));    /* x */
  rb_define_const(og_mOniguruma, "OPTION_MULTILINE",          INT2FIX(4));    /* m */
  rb_define_const(og_mOniguruma, "OPTION_SINGLELINE",         INT2FIX(8));    /* s */
  rb_define_const(og_mOniguruma, "OPTION_FIND_LONGEST",       INT2FIX(16));   /* l */
  rb_define_const(og_mOniguruma, "OPTION_FIND_NOT_EMPTY",     INT2FIX(32));   /* E */
  rb_define_const(og_mOniguruma, "OPTION_NEGATE_SINGLELINE",  INT2FIX(64));   /* S */
  rb_define_const(og_mOniguruma, "OPTION_DONT_CAPTURE_GROUP", INT2FIX(128));  /* G */
  rb_define_const(og_mOniguruma, "OPTION_CAPTURE_GROUP",      INT2FIX(256));  /* g */
  rb_define_const(og_mOniguruma, "OPTION_NOTBOL",             INT2FIX(512));  /* B */
  rb_define_const(og_mOniguruma, "OPTION_NOTEOL",             INT2FIX(1024)); /* E */
  rb_define_const(og_mOniguruma, "OPTION_POSIX_REGION",       INT2FIX(2048));
  rb_define_const(og_mOniguruma, "OPTION_MAXBIT",             INT2FIX(4096));
  
  og_mOniguruma_Opt_Shortcuts = rb_hash_new();
  rb_hash_aset(og_mOniguruma_Opt_Shortcuts, rb_str_new2("i"), rb_const_get(og_mOniguruma, rb_intern("OPTION_IGNORECASE")));
  rb_hash_aset(og_mOniguruma_Opt_Shortcuts, rb_str_new2("x"), rb_const_get(og_mOniguruma, rb_intern("OPTION_EXTEND")));
  rb_hash_aset(og_mOniguruma_Opt_Shortcuts, rb_str_new2("m"), rb_const_get(og_mOniguruma, rb_intern("OPTION_MULTILINE")));
  rb_hash_aset(og_mOniguruma_Opt_Shortcuts, rb_str_new2("s"), rb_const_get(og_mOniguruma, rb_intern("OPTION_SINGLELINE")));
  rb_hash_aset(og_mOniguruma_Opt_Shortcuts, rb_str_new2("l"), rb_const_get(og_mOniguruma, rb_intern("OPTION_FIND_LONGEST")));
  rb_hash_aset(og_mOniguruma_Opt_Shortcuts, rb_str_new2("E"), rb_const_get(og_mOniguruma, rb_intern("OPTION_FIND_NOT_EMPTY")));
  rb_hash_aset(og_mOniguruma_Opt_Shortcuts, rb_str_new2("S"), rb_const_get(og_mOniguruma, rb_intern("OPTION_NEGATE_SINGLELINE")));
  rb_hash_aset(og_mOniguruma_Opt_Shortcuts, rb_str_new2("G"), rb_const_get(og_mOniguruma, rb_intern("OPTION_DONT_CAPTURE_GROUP")));
  rb_hash_aset(og_mOniguruma_Opt_Shortcuts, rb_str_new2("g"), rb_const_get(og_mOniguruma, rb_intern("OPTION_CAPTURE_GROUP")));
  rb_hash_aset(og_mOniguruma_Opt_Shortcuts, rb_str_new2("B"), rb_const_get(og_mOniguruma, rb_intern("OPTION_NOTBOL")));
  rb_hash_aset(og_mOniguruma_Opt_Shortcuts, rb_str_new2("E"), rb_const_get(og_mOniguruma, rb_intern("OPTION_NOTEOL")));
  
  OBJ_FREEZE(og_mOniguruma_Opt_Shortcuts);
  
  rb_define_const(og_mOniguruma, "OPT_SHORTCUTS", og_mOniguruma_Opt_Shortcuts);
}

OnigEncodingType*
og_oniguruma_extract_encoding(VALUE encoding)
{
  int key = FIX2INT(encoding);
  
  switch (key) {
    case 1:  return ONIG_ENCODING_ASCII;
    case 2:  return ONIG_ENCODING_ISO_8859_1;
    case 3:  return ONIG_ENCODING_ISO_8859_2;
    case 4:  return ONIG_ENCODING_ISO_8859_3;
    case 5:  return ONIG_ENCODING_ISO_8859_4;
    case 6:  return ONIG_ENCODING_ISO_8859_5;
    case 7:  return ONIG_ENCODING_ISO_8859_6;
    case 8:  return ONIG_ENCODING_ISO_8859_7;
    case 9:  return ONIG_ENCODING_ISO_8859_8;
    case 10: return ONIG_ENCODING_ISO_8859_9;
    case 11: return ONIG_ENCODING_ISO_8859_10;
    case 12: return ONIG_ENCODING_ISO_8859_11;
    case 13: return ONIG_ENCODING_ISO_8859_13;
    case 14: return ONIG_ENCODING_ISO_8859_14;
    case 15: return ONIG_ENCODING_ISO_8859_15;
    case 16: return ONIG_ENCODING_ISO_8859_16;
    case 17: return ONIG_ENCODING_UTF8;
    case 18: return ONIG_ENCODING_EUC_JP;
    case 19: return ONIG_ENCODING_EUC_TW;
    case 20: return ONIG_ENCODING_EUC_KR;
    case 21: return ONIG_ENCODING_EUC_CN;
    case 22: return ONIG_ENCODING_SJIS;
    case 23: return ONIG_ENCODING_KOI8;
    case 24: return ONIG_ENCODING_KOI8_R;
    case 25: return ONIG_ENCODING_BIG5;
    default: return ONIG_ENCODING_UNDEF;
  }
}

OnigSyntaxType*
og_oniguruma_extract_syntax(VALUE syntax)
{
  int key = FIX2INT(syntax);
  
  switch (key) {
    case 1:  return ONIG_SYNTAX_POSIX_BASIC;
    case 2:  return ONIG_SYNTAX_POSIX_EXTENDED;
    case 3:  return ONIG_SYNTAX_EMACS;
    case 4:  return ONIG_SYNTAX_GREP;
    case 5:  return ONIG_SYNTAX_GNU_REGEX;
    case 6:  return ONIG_SYNTAX_JAVA;
    case 7:  return ONIG_SYNTAX_PERL;
    case 8:  return ONIG_SYNTAX_RUBY;
    default: return ONIG_SYNTAX_DEFAULT;
  }
}
