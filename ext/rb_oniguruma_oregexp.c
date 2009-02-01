#include "rb_oniguruma.h"
#include "rb_oniguruma_match.h"
#include "rb_oniguruma_struct_args.h"

#if ONIGURUMA_VERSION_MAJOR >= 5
# ifndef enc_len
#  define enc_len(enc, byte) ONIGENC_MBC_ENC_LEN(enc, byte)
# endif
#endif

#define og_oniguruma_oregexp_get_code_point(cp, cpl, enc, rep, pos) do {    \
  cp = ONIGENC_MBC_TO_CODE(enc, OG_STRING_PTR(rep) + pos,                   \
    OG_STRING_PTR(rep) + RSTRING(rep)->len - 1);                            \
  cpl = enc_len(enc, (OG_STRING_PTR(rep) + pos));                           \
} while(0)

static inline void
og_oniguruma_string_modification_check(VALUE s, char *p, long len)
{
  if (RSTRING(s)->ptr != p || RSTRING(s)->len != len)
    rb_raise(rb_eRuntimeError, "string modified");
}

#pragma mark Class Methods

/*
 * Document-method: escape
 *
 * call-seq:
 * ORegexp.escape(str)   => a_str
 * ORegexp.quote(str)    => a_str
 *
 * Escapes any characters that would have special meaning in a regular
 * expression. Returns a new escaped string, or self if no characters are
 * escaped.  For any string,
 * <code>ORegexp.escape(<i>str</i>)=~<i>str</i></code> will be true.
 *
 *    ORegexp.escape('\\*?{}.')   #=> \\\\\*\?\{\}\.
 */
static VALUE
og_oniguruma_oregexp_escape(int argc, VALUE *argv, VALUE self)
{
  return rb_funcall3(rb_cRegexp, rb_intern("escape"), argc, argv);
}

/*
 * Document-method: last_match
 *
 * call-seq:
 *    ORegexp.last_match           => matchdata
 *    ORegexp.last_match(fixnum)   => str
 *
 * The first form returns the <code>MatchData</code> object generated by the
 * last successful pattern match. The second form returns the nth field in this
 * <code>MatchData</code> object.
 *
 *    ORegexp.new( 'c(.)t' ) =~ 'cat'       #=> 0
 *    ORegexp.last_match                    #=> #<MatchData:0x401b3d30>
 *    ORegexp.last_match(0)                 #=> "cat"
 *    ORegexp.last_match(1)                 #=> "a"
 *    ORegexp.last_match(2)                 #=> nil
 */
static VALUE
og_oniguruma_oregexp_last_match(int argc, VALUE *argv, VALUE self)
{
  VALUE index, args[2];
  
  rb_scan_args(argc, argv, "01", &index);
  
  if (index == Qnil) {
    return rb_cv_get(self, "@@last_match");
  } else {
    args[0] = index;
    args[1] = (VALUE)NULL;
    
    return rb_funcall3(rb_cv_get(self, "@@last_match"), rb_intern("[]"), 1, args);
  }
}

/* Constructor Methods */
static void
og_oniguruma_oregexp_free(void *arg)
{
  og_ORegexp *oregexp = (og_ORegexp*)arg;
  onig_free(oregexp->reg);
  free(oregexp);
}

/*
 * Document-method: initialize
 *
 * call-seq:
 *     ORegexp.new( pattern, options_hash )
 *     ORegexp.new( pattern, option_str, encoding_str=nil, syntax_str=nil) 
 *
 * Constructs a new regular expression from <i>pattern</i>, which is a 
 * <code>String</code>. The second parameter <i></i> may be a <code>Hash</code> 
 * of the form:
 *
 * <code>{ :options => option_value, :encoding => encoding_value, :syntax => syntax_value }</code>
 *
 * Where <code>option_value</code> is a bitwise <code>OR</code> of 
 * <code>Oniguruma::OPTION_XXX</code> constants; <code>encoding_value</code>
 * is one of <code>Oniguruma::ENCODING_XXX</code> constants; and 
 * <code>syntax_value</code> is one of <code>Oniguruma::SYNTAX_XXX</code>
 * constants.
 *
 *     r1 = ORegexp.new('^a-z+:\\s+\w+')                                            #=> /^a-z+:\s+\w+/
 *     r2 = ORegexp.new('cat', :options => OPTION_IGNORECASE )                      #=> /cat/i
 *     r3 = ORegexp.new('dog', :options => OPTION_EXTEND )                          #=> /dog/x
 *
 *     #Accept java syntax on SJIS encoding:
 *     r4 = ORegexp.new('ape', :syntax  => SYNTAX_JAVA, :encoding => ENCODING_SJIS) #=> /ape/ 
 *
 * Second form uses string shortcuts to set options and encoding:
 *     r = ORegexp.new('cat', 'i', 'utf8', 'java')
 */
static VALUE
og_oniguruma_oregexp_alloc(VALUE klass)
{
  VALUE obj;
  og_ORegexp *oregexp;
  
  oregexp = malloc( sizeof( og_ORegexp ) );
  oregexp->reg = NULL;
  
  obj = Data_Wrap_Struct(klass, 0, og_oniguruma_oregexp_free, oregexp);
  return obj;
}

/* Instance Methods */
static int
og_oniguruma_oregexp_compile(VALUE self, VALUE regex)
{
  int result;
  og_ORegexp *oregexp;
  OnigErrorInfo error_info;
  UChar error_string[ONIG_MAX_ERROR_MESSAGE_LEN];
  
  Data_Get_Struct(self, og_ORegexp, oregexp);
  StringValue(regex);
  
  result = onig_new(&(oregexp->reg),    /* Regexp Object */
    OG_STRING_PTR(regex), OG_STRING_PTR(regex) + RSTRING(regex)->len,
    og_oniguruma_extract_option(rb_iv_get(self, "@options")),
    og_oniguruma_extract_encoding(rb_iv_get(self, "@encoding")),
    og_oniguruma_extract_syntax(rb_iv_get(self, "@syntax")),
    &error_info);
  
  if (result != ONIG_NORMAL) {
    onig_error_code_to_str(error_string, result, &error_info);
    rb_raise(rb_eArgError, "Oniguruma Error: %s", error_string);
  }
  
  return Qnil;
}

static void
og_oniguruma_oregexp_options_parse(VALUE self, VALUE hash)
{
  VALUE options, encoding, syntax, og_mOniguruma;
  
  og_mOniguruma = rb_const_get(rb_cObject, rb_intern(OG_M_ONIGURUMA));
  
  encoding = rb_hash_aref(hash, ID2SYM(rb_intern("encoding")));
  options  = rb_hash_aref(hash, ID2SYM(rb_intern("options")));
  syntax   = rb_hash_aref(hash, ID2SYM(rb_intern("syntax")));
  
  if (NIL_P(encoding))
    encoding = rb_const_get(og_mOniguruma, rb_intern("ENCODING_ASCII"));
  
  if (NIL_P(options))
    options = rb_const_get(og_mOniguruma, rb_intern("OPTION_DEFAULT"));
  
  if (NIL_P(syntax))
    syntax = rb_const_get(og_mOniguruma, rb_intern("SYNTAX_DEFAULT"));
  
  rb_iv_set(self, "@encoding", encoding);
  rb_iv_set(self, "@options", options);
  rb_iv_set(self, "@syntax", syntax);
}

static VALUE
og_oniguruma_oregexp_initialize_real(VALUE self, VALUE re, VALUE options)
{
  rb_iv_set(self, "@pattern", StringValue(re)); /* Take a copy */
  og_oniguruma_oregexp_options_parse(self, options);
  og_oniguruma_oregexp_compile(self, rb_iv_get(self, "@pattern"));
  
  return self;
}

static void
og_oniguruma_oregexp_upper(char *str)
{
  while (*str != '\0')
  {
    if ('a' <= *str && *str <= 'z')
      *str -= 32;
    str++;
  }
}

static VALUE
og_oniguruma_oregexp_initialize(int argc, VALUE *argv, VALUE self)
{
  int i;
  long opts;
  char *byte;
  VALUE re, args, options, shortcuts,
    cut, opt, enc, syn, og_mOniguruma;

  og_mOniguruma = rb_const_get(rb_cObject, rb_intern(OG_M_ONIGURUMA));
  
  rb_scan_args(argc, argv, "1*", &re, &args);
  
  if (TYPE(rb_ary_entry(args, 0)) == T_STRING) {
    options = rb_hash_new();
    shortcuts = rb_const_get(og_mOniguruma, rb_intern("OPT_SHORTCUTS"));
    
    opt = rb_ary_shift(args);
    enc = rb_ary_shift(args);
    syn = rb_ary_shift(args);
    
    opts = 0;
    
    if (!NIL_P(opt)) {
      for (i = 0, byte = RSTRING(opt)->ptr; i < RSTRING(opt)->len; i++, byte++)
      {
        cut = rb_hash_aref(shortcuts, rb_str_new(byte, 1));
        if (!NIL_P(cut))
          opts = opts | FIX2INT(cut);
      }
    }
    
    rb_hash_aset(options, ID2SYM(rb_intern("options")), INT2FIX(opts));
    
    if (!NIL_P(enc)) {
      byte = (char *) malloc( sizeof(char) * (RSTRING(enc)->len + 10));
      snprintf(byte, RSTRING(enc)->len + 10, "ENCODING_%s", RSTRING(enc)->ptr);
      og_oniguruma_oregexp_upper(byte);
    
      if (!NIL_P(enc) && rb_const_defined(og_mOniguruma, rb_intern(byte)))
        rb_hash_aset(options, ID2SYM(rb_intern("encoding")), rb_const_get(og_mOniguruma, rb_intern(byte)));
    
      free(byte);
    }
    
    if (!NIL_P(syn)) {
      byte = (char *) malloc( sizeof(char) * (RSTRING(syn)->len + 8));
      snprintf(byte, RSTRING(syn)->len + 10, "SYNTAX_%s", RSTRING(syn)->ptr);
      og_oniguruma_oregexp_upper(byte);
    
      if (!NIL_P(syn) && rb_const_defined(og_mOniguruma, rb_intern(byte)))
        rb_hash_aset(options, ID2SYM(rb_intern("syntax")), rb_const_get(og_mOniguruma, rb_intern(byte)));
    
      free(byte);
    }
  } else {
    if (!NIL_P(rb_ary_entry(args, 0)))
      options = rb_ary_shift(args);
    else
      options = rb_hash_new();
  }
  
  return og_oniguruma_oregexp_initialize_real(self, re, options);
}

static VALUE
og_oniguruma_oregexp_do_match(VALUE self, OnigRegion *region, VALUE string)
{
  VALUE match;
  og_CallbackPacket packet;
  og_ORegexp *oregexp;
  
  Data_Get_Struct(self, og_ORegexp, oregexp);
  
  match = og_oniguruma_match_initialize(region, string);
  
  rb_cv_set(CLASS_OF(self), "@@last_match", match);
  
  packet.region = region;
  
  if (onig_number_of_names(oregexp->reg) > 0) {
    packet.hash = rb_hash_new();
    onig_foreach_name(oregexp->reg, &og_oniguruma_name_callback, &packet);
    rb_iv_set(match, "@named_captures", packet.hash);
  }
  
  return match;
}

/*
 * Document-method: match
 *
 * call-seq:
 *    rxp.match(str)               => matchdata or nil
 *    rxp.match(str, begin, end)   => matchdata or nil
 *
 * Returns a <code>MatchData</code> object describing the match, or
 * <code>nil</code> if there was no match. This is equivalent to retrieving the
 * value of the special variable <code>$~</code> following a normal match.
 *
 *    ORegexp.new('(.)(.)(.)').match("abc")[2]   #=> "b"
 *
 * The second form allows to perform the match in a region
 * defined by <code>begin</code> and <code>end</code> while
 * still taking into account look-behinds and look-forwards.
 *
 *    ORegexp.new('1*2*').match('11221122').offset       => [4,8]
 *    ORegexp.new('(?<=2)1*2*').match('11221122').offset => [4,8]
 *
 * Compare with:
 *
 *    ORegexp.new('(?<=2)1*2*').match('11221122'[4..-1]) => nil
 */
static VALUE
og_oniguruma_oregexp_match(int argc, VALUE *argv, VALUE self)
{
  int result;
  OnigRegion *region;
  og_ORegexp *oregexp;
  
  UChar error_string[ONIG_MAX_ERROR_MESSAGE_LEN];
  
  VALUE string, begin, end, match;
  
  rb_scan_args(argc, argv, "12", &string, &begin, &end);
  
  if (NIL_P(begin)) begin = INT2FIX(0);
  if (NIL_P(end))   end   = INT2FIX(RSTRING(string)->len);
  
  Data_Get_Struct(self, og_ORegexp, oregexp);
  
  StringValue(string);
  
  region = onig_region_new();
  result = onig_search(oregexp->reg,
    OG_STRING_PTR(string),  OG_STRING_PTR(string) + RSTRING(string)->len,
    OG_STRING_PTR(string) + FIX2INT(begin),  OG_STRING_PTR(string) + FIX2INT(end),
    region, ONIG_OPTION_NONE);
  
  rb_backref_set(Qnil);
  if (result >= 0) {
    match = og_oniguruma_oregexp_do_match(self, region, string);
    
    onig_region_free(region, 1);
    rb_backref_set(match);
    rb_match_busy(match);
    
    return match;
  } else if (result == ONIG_MISMATCH) {
    onig_region_free(region, 1);
  } else {
    onig_region_free(region, 1);
    
    onig_error_code_to_str(error_string, result);
    rb_raise(rb_eArgError, OG_M_ONIGURUMA " Error: %s", error_string);
  }
  
  return Qnil;
}

static VALUE
og_oniguruma_oregexp_do_replacement(VALUE self, VALUE buffer, VALUE str, VALUE replacement, OnigRegion *region)
{
  const UChar *subj; long subj_len;
  og_ORegexp *oregexp;
  
  int position = 0, digits, group, named_group_pos,
    named_group_begin, named_group_end,
    code_point_len, previous_code_point_len;
  OnigCodePoint code_point;
  OnigEncoding encoding;
  
  subj = OG_STRING_PTR(str); subj_len = RSTRING(str)->len;
  Data_Get_Struct(self, og_ORegexp, oregexp);
  
  encoding = onig_get_encoding(oregexp->reg);
  
  while (position < RSTRING(replacement)->len)
  {
    og_oniguruma_oregexp_get_code_point(code_point, code_point_len, encoding, replacement, position);
    
    if (code_point_len == 0) {
      rb_warn("Anomally: length of char '%d' is 0", code_point);
      code_point_len = 1;
    }
    
    position += code_point_len;
    
    if (code_point != 0x5c) { /* 0x5c is a backslash \ */
      rb_str_buf_cat(buffer, RSTRING(replacement)->ptr + position - code_point_len, code_point_len);
      continue;
    }
    
    if (position >= RSTRING(replacement)->len) {
      rb_str_buf_cat(buffer, RSTRING(replacement)->ptr + (position - code_point_len), code_point_len);
      break;
    }
    
    digits = group = 0;
    while(1) /* n + 1/2 times loop. break out manually. */
    {
      if (position >= RSTRING(replacement)->len) break;
      
      og_oniguruma_oregexp_get_code_point(code_point, code_point_len, encoding, replacement, position);
      
      if (!ONIGENC_IS_CODE_DIGIT(encoding, code_point)) break;
      
      group = group * 10 + (code_point - '0');
      position += code_point_len;
      digits++;
      
      if (digits >= 2) break; /* limit 99 groups */
    }
    
    if (digits == 0) {
      previous_code_point_len = code_point_len;
      og_oniguruma_oregexp_get_code_point(code_point, code_point_len, encoding, replacement, position);
      
      switch (code_point)
      {
        case '\\': // \ literal, just cat and keep going
          rb_str_buf_cat(buffer, RSTRING(replacement)->ptr + position, code_point_len);
          position += code_point_len;
          break;
        
        case '&': // matched substring
          rb_str_buf_cat(buffer, (char*)(subj + region->beg[0]), region->end[0] - region->beg[0]);
          position += code_point_len;
          break;
        
        case '`': // prematch
          rb_str_buf_cat(buffer, (char*)subj, region->beg[0]);
          position += code_point_len;
          break;
        
        case '\'': // postmatch
          rb_str_buf_cat(buffer, (char*)(subj + region->end[0]), subj_len - region->end[0]);
          position += code_point_len;
          break;
          
        case '+': // last matched
          position += code_point_len;
          
          for (group = region->num_regs - 1; group > 0; group--)
          {
            if (region->beg[group] != -1) {
              rb_str_buf_cat(buffer, (char*)(subj + region->beg[group]),
                region->end[group] - region->beg[group]);
              break;
            }
          }
          
          break;
        
        //case 'k': // Oniguruma style named group reference
          // Eat the 'k' code point and let it fall through to the '<'
          //og_oniguruma_oregexp_get_code_point(code_point, code_point_len, encoding, replacement, position);
          //position += code_point_len;
        case '<': // Oniguruma Gem named group reference (for compatibility)
          named_group_end = named_group_begin = named_group_pos = position + code_point_len;
          
          while (named_group_pos < RSTRING(replacement)->len)
          {
            og_oniguruma_oregexp_get_code_point(code_point, code_point_len, encoding, replacement, named_group_pos);
            named_group_pos += code_point_len;
            
            if (code_point == '>') break;
            
            if (ONIGENC_IS_CODE_WORD(encoding, code_point))
              named_group_end += code_point_len;
            else break;
          }
          
          if (code_point != '>' || named_group_end == named_group_begin) { // place backslash and '<'
            rb_str_buf_cat(buffer, RSTRING(replacement)->ptr + (position - previous_code_point_len),
              previous_code_point_len + code_point_len);
            position += code_point_len;
          } else { // lookup for group and subst for that value
            group = onig_name_to_backref_number(oregexp->reg, 
              OG_STRING_PTR(replacement) + named_group_begin,
              OG_STRING_PTR(replacement) + named_group_end, region);
            
            if (group >= 0)
              rb_str_buf_cat(buffer, (char*)(subj + region->beg[group]),
                region->end[group] - region->beg[group]);
            position = named_group_pos;
          }
          break;

        default:
          rb_str_buf_cat(buffer, RSTRING(replacement)->ptr + (position - previous_code_point_len),
            previous_code_point_len + code_point_len);
          position += code_point_len;
          break;
      } /* switch (code_point) */
    } else {
      if (group < region->num_regs && region->beg[group] >= 0)
        rb_str_buf_cat(buffer, (char*)(subj + region->beg[group]),
          region->end[group] - region->beg[group]);
    } /* if (digits == 0) */
  } /* while (position < RSTRING(replacement)->len) */
  
  return Qnil;
}

static VALUE
og_oniguruma_oregexp_do_substitution(og_SubstitutionArgs *args)
{
  int tainted_replacement = 0;
  VALUE str, replacement, block;
  og_ORegexp *oregexp;
  
  long begin = 0, end = 0, last_end = 0, multibyte_diff = 0;
  
  UChar *subj; int subj_len;
  VALUE buffer, block_match, block_result;
  OnigEncoding encoding;
  
  /* Parse the arguments */
  if (rb_block_given_p()) {
    rb_scan_args(args->argc, args->argv, "1&", &str, &block);
  } else {
    rb_scan_args(args->argc, args->argv, "2", &str, &replacement);
    Check_Type(replacement, T_STRING);
    if (OBJ_TAINTED(replacement))
      tainted_replacement = 1;
  }
  
  // Ensure str is a string
  StringValue(str);
  
  Data_Get_Struct(args->self, og_ORegexp, oregexp);
  subj = OG_STRING_PTR(str); subj_len = RSTRING(str)->len;
  
  begin = onig_search(oregexp->reg,
    subj, subj + subj_len,
    subj, subj + subj_len,
    args->region, ONIG_OPTION_NONE);
  
  if (begin < 0) {
    if (args->update_self)
      return Qnil;
    return rb_str_dup(str);
  }
  
  buffer = rb_str_buf_new(subj_len);
  encoding = onig_get_encoding(oregexp->reg);
  
  do {
    last_end = end;
    begin = args->region->beg[0];
    end   = args->region->end[0];
    
    rb_str_buf_cat(buffer, (char*)(subj + last_end), begin - last_end);
    
    if (rb_block_given_p()) {
      /* yielding to a block */
      block_match = og_oniguruma_oregexp_do_match(args->self, args->region, str);
      
      rb_backref_set(block_match);
      rb_match_busy(block_match);
      
      block_result = rb_yield(block_match);
      
      og_oniguruma_string_modification_check(str, (char*)subj, subj_len);
      replacement = rb_obj_as_string(block_result);
      rb_str_append(buffer, replacement);
    } else {
      og_oniguruma_oregexp_do_replacement(args->self, buffer, str, replacement, args->region);
    }
    
    if (!args->global) break;
    
    /* Finish this match so we can do the next one */
    if (begin == end) {
      if (subj_len <= end) break;
      
      multibyte_diff = enc_len(encoding, (subj + end));
      rb_str_buf_cat(buffer, (char*)(subj + end), multibyte_diff);
      end += multibyte_diff;
    }
    
    begin = onig_search(oregexp->reg,
      subj,       subj + subj_len,
      subj + end, subj + subj_len,
      args->region, ONIG_OPTION_NONE);
  } while (begin >= 0);
  
  rb_str_buf_cat(buffer, (char*)(subj + end), subj_len - end);
  
  if (tainted_replacement)
    OBJ_INFECT(buffer, replacement);
  OBJ_INFECT(buffer, str);
  
  if (args->update_self) {
    rb_funcall(str, rb_intern("replace"), 1, buffer);
    return str;
  }
  
  return buffer;
}

static VALUE
og_oniguruma_oregexp_do_cleanup(OnigRegion *region)
{
  onig_region_free(region, 1);
  return Qnil;
}

static VALUE
og_oniguruma_oregexp_do_substitution_safe(VALUE self,
  int argc, VALUE *argv, int global, int update_self)
{
  OnigRegion *region = onig_region_new();
  og_SubstitutionArgs fargs;
  
  og_SubstitutionArgs_set(&fargs, self, argc, argv, global, update_self, region);
  return rb_ensure(og_oniguruma_oregexp_do_substitution, (VALUE)&fargs,
    og_oniguruma_oregexp_do_cleanup, (VALUE)region);
}

/*
 * Document-method: gsub
 *
 * call-seq:
 *     rxp.gsub(str, replacement)
 *     rxp.gsub(str) {|match_data| ... }
 *
 * Returns a copy of _str_ with _all_ occurrences of _rxp_ pattern
 * replaced with either _replacement_ or the value of the block.
 *
 * If a string is used as the replacement, the sequences \1, \2,
 * and so on may be used to interpolate successive groups in the match.
 *
 * In the block form, the current MatchData object is passed in as a
 * parameter. The value returned by the block will be substituted for
 * the match on each call.
 */
static VALUE
og_oniguruma_oregexp_gsub(int argc, VALUE *argv, VALUE self)
{
  return og_oniguruma_oregexp_do_substitution_safe(self, argc, argv, 1, 0);
}

/*
 * Document-method: gsub!
 *
 * call-seq:
 *     rxp.gsub!(str, replacement)
 *     rxp.gsub!(str) {|match_data| ... }
 *
 * Performs the substitutions of ORegexp#gsub in place, returning
 * _str_, or _nil_ if no substitutions were performed
 */
static VALUE
og_oniguruma_oregexp_gsub_bang(int argc, VALUE *argv, VALUE self)
{
  return og_oniguruma_oregexp_do_substitution_safe(self, argc, argv, 1, 1);
}

/*
 * Document-method: sub
 *
 * call-seq:
 *     rxp.sub(str, replacement)
 *     rxp.sub(str) {|match_data| ... }
 *
 * Returns a copy of _str_ with the _first_ occurrence of _rxp_ pattern
 * replaced with either _replacement_ or the value of the block.
 *
 * If a string is used as the replacement, the sequences \1, \2,
 * and so on may be used to interpolate successive groups in the match.
 *
 * In the block form, the current MatchData object is passed in as a
 * parameter. The value returned by the block will be substituted for
 * the match on each call.
 */
static VALUE
og_oniguruma_oregexp_sub(int argc, VALUE *argv, VALUE self)
{
  return og_oniguruma_oregexp_do_substitution_safe(self, argc, argv, 0, 0);
}

/*
 * Document-method: sub!
 *
 * call-seq:
 *     oregexp.sub!(str, replacement)
 *     oregexp.sub!(str) {|match_data| ... }
 *
 * Performs the substitutions of ORegexp#sub in place, returning
 * _str_, or _nil_ if no substitutions were performed.
 *
 */
static VALUE
og_oniguruma_oregexp_sub_bang(int argc, VALUE *argv, VALUE self)
{
  return og_oniguruma_oregexp_do_substitution_safe(self, argc, argv, 0, 1);
}

static VALUE
og_oniguruma_oregexp_do_scan(og_ScanArgs *args)
{ 
  VALUE str, match, matches;
  OnigEncoding encoding;
  og_ORegexp *oregexp;
  long begin = 0, end = 0, multibyte_diff = 0;
  
  Data_Get_Struct(args->self, og_ORegexp, oregexp);
  
  str = StringValue(args->str);
  
  begin = onig_search(oregexp->reg,
    OG_STRING_PTR(str), OG_STRING_PTR(str) + RSTRING(str)->len,
    OG_STRING_PTR(str), OG_STRING_PTR(str) + RSTRING(str)->len,
    args->region, ONIG_OPTION_NONE);
    
  if (begin < 0)
    return Qnil;
  
  matches = rb_ary_new();
  encoding = onig_get_encoding(oregexp->reg);
  
  do {
    match = og_oniguruma_oregexp_do_match(args->self, args->region, str);
    end = args->region->end[0];
    rb_ary_push(matches, match);
    
    if (rb_block_given_p())
      rb_yield(match);
    
    if (end == begin) {
      if( RSTRING(str)->len <= end )
        break;
      multibyte_diff = enc_len(encoding, OG_STRING_PTR(str) + end);
      end += multibyte_diff;
    }
    
    begin = onig_search(oregexp->reg,
      OG_STRING_PTR(str),       OG_STRING_PTR(str) + RSTRING(str)->len,
      OG_STRING_PTR(str) + end, OG_STRING_PTR(str) + RSTRING(str)->len,
      args->region, ONIG_OPTION_NONE);
  } while (begin >= 0);
  
  return matches;
}

/*
 * Document-method: scan
 *
 * call-seq:
 *     rxp.scan(str)                        # => [matchdata1, matchdata2,...] or nil
 *     rxp.scan(str) {|match_data| ... }    # => [matchdata1, matchdata2,...] or nil
 *
 * Both forms iterate through _str_, matching the pattern. For each match,
 * a MatchData object is generated and passed to the block, and
 * added to the resulting array of MatchData objects.
 *
 * If _str_ does not match pattern, _nil_ is returned.
 */
static VALUE
og_oniguruma_oregexp_scan(VALUE self, VALUE str)
{
  OnigRegion *region = onig_region_new();
  og_ScanArgs fargs;
  
  og_ScanArgs_set(&fargs, self, str, region);
  return rb_ensure(og_oniguruma_oregexp_do_scan, (VALUE)&fargs,
    og_oniguruma_oregexp_do_cleanup, (VALUE)region);
}

/*
 * Document-method: casefold?
 *
 * call-seq:
 *    rxp.casefold?   => true of false
 *
 * Returns the value of the case-insensitive flag.
 */
static VALUE
og_oniguruma_oregexp_casefold(VALUE self)
{
  int options, ignore_case;
  VALUE og_mOniguruma;
  
  og_mOniguruma = rb_const_get(rb_cObject, rb_intern(OG_M_ONIGURUMA));
  
  options = FIX2INT(rb_iv_get(self, "@options"));
  ignore_case = FIX2INT(rb_const_get(og_mOniguruma, rb_intern("OPTION_IGNORECASE")));
  
  if ((options & ignore_case) > 0)
    return Qtrue;
  return Qfalse;
}

/*
 * Document-method: ==
 *
 * call-seq:
 *    rxp == other_rxp      => true or false
 *    rxp.eql?(other_rxp)   => true or false
 *
 * Equality---Two regexps are equal if their patterns are identical, they have
 * the same character set code, and their <code>#casefold?</code> values are the
 * same.
 */
static VALUE
og_oniguruma_oregexp_operator_equality(VALUE self, VALUE rhs)
{
  VALUE pattern, encoding, rhs_pattern, rhs_encoding;
  
  /* TODO: test type, make sure its an Oniguruma::ORegexp object */
  
  encoding = rb_iv_get(self, "@encoding");
  pattern = rb_iv_get(self, "@pattern");
  
  rhs_encoding = rb_iv_get(rhs, "@encoding");
  rhs_pattern = rb_iv_get(rhs, "@pattern");
  
  if (rb_str_cmp(pattern, rhs_pattern) == 0    &&
    FIX2INT(encoding) == FIX2INT(rhs_encoding) &&
    og_oniguruma_oregexp_casefold(self) == og_oniguruma_oregexp_casefold(rhs))
      return Qtrue;
  return Qfalse;
}

/*
 * Document-method: ===
 *
 * call-seq:
 *     rxp === str   => true or false
 *
 * Case Equality---Synonym for <code>ORegexp#=~</code> used in case statements.
 *
 *    a = "HELLO"
 *    case a
 *    when ORegexp.new('^[a-z]*$'); print "Lower case\n"
 *    when ORegexp.new('^[A-Z]*$'); print "Upper case\n"
 *    else;                         print "Mixed case\n"
 *    end
 *
 * <em>produces:</em>
 *
 *    Upper case
 */
static VALUE
og_oniguruma_oregexp_operator_identical(VALUE self, VALUE str)
{
  VALUE match, args[2];
  
  if (TYPE(str) != T_STRING) {
    str = rb_check_string_type(str);
    if (NIL_P(str))
      return Qfalse;
  }
  
  args[0] = StringValue(str);
  args[1] = (VALUE)NULL;
  
  match = og_oniguruma_oregexp_match(1, args, self);
  
  if (NIL_P(match))
    return Qfalse;
  return Qtrue;
}

/*
 * Document-method: =~
 *
 * call-seq:
 *    rxp =~ string  => int or nil
 *
 * Matches <code>rxp</code> against <code>string</code>, returning the offset
 * of the start of the match or <code>nil</code> if the match failed. Sets $~
 * to the corresponding <code>MatchData</code> or <code>nil</code>.
 *
 *    ORegexp.new( 'SIT' ) =~ "insensitive"                                 #=>    nil
 *    ORegexp.new( 'SIT', :options => OPTION_IGNORECASE ) =~ "insensitive"  #=>    5
 */
static VALUE
og_oniguruma_oregexp_operator_match(VALUE self, VALUE str)
{
  VALUE match, args[2];
  
  args[0] = str;
  args[1] = (VALUE)NULL;
  
  match = og_oniguruma_oregexp_match(1, args, self);
  
  if(NIL_P(match))
    return Qnil;
  return INT2FIX(RMATCH(match)->regs->beg[0]);
}

/*
 * Document-method: kcode
 *
 * call-seq:
 *    rxp.kode        => int
 *
 * Returns the character set code for the regexp.
 */
static VALUE
og_oniguruma_oregexp_kcode(VALUE self)
{
  return rb_iv_get(self, "@encoding");
}

/*
 * Document-method: options
 *
 * call-seq:
 *    rxp.options   => fixnum
 *
 * Returns the set of bits corresponding to the options used when creating this
 * ORegexp (see <code>ORegexp::new</code> for details. Note that additional bits
 * may be set in the returned options: these are used internally by the regular
 * expression code. These extra bits are ignored if the options are passed to
 * <code>ORegexp::new</code>.
 *
 *    Oniguruma::OPTION_IGNORECASE                                 #=> 1
 *    Oniguruma::OPTION_EXTEND                                     #=> 2
 *    Oniguruma::OPTION_MULTILINE                                  #=> 4
 *
 *    Regexp.new(r.source, :options => Oniguruma::OPTION_EXTEND )  #=> 2
 */
static VALUE
og_oniguruma_oregexp_options(VALUE self)
{
  return rb_iv_get(self, "@options");
}

/*
 * Document-method: source
 *
 * call-seq:
 *    rxp.source   => str
 *
 * Returns the original string of the pattern.
 *
 *    ORegex.new( 'ab+c', 'ix' ).source   #=> "ab+c"
 */
static VALUE
og_oniguruma_oregexp_source(VALUE self)
{
  VALUE pattern = rb_iv_get(self, "@pattern");
  OBJ_FREEZE(pattern);
  return pattern;
}

/*
 * Document-method: to_s
 *
 * call-seq:
 *    rxp.to_s   => str
 *
 * Returns a string containing the regular expression and its options (using the
 * <code>(?xxx:yyy)</code> notation. This string can be fed back in to
 * <code>Regexp::new</code> to a regular expression with the same semantics as
 * the original. (However, <code>Regexp#==</code> may not return true when
 * comparing the two, as the source of the regular expression itself may
 * differ, as the example shows).  <code>Regexp#inspect</code> produces a
 * generally more readable version of <i>rxp</i>.
 *
 *    r1 = ORegexp.new( 'ab+c', :options OPTION_IGNORECASE | OPTION_EXTEND ) #=> /ab+c/ix
 *    s1 = r1.to_s                                                           #=> "(?ix-m:ab+c)"
 *    r2 = ORegexp.new(s1)                                                   #=> /(?ix-m:ab+c)/
 *    r1 == r2                                                               #=> false
 *    r1.source                                                              #=> "ab+c"
 *    r2.source                                                              #=> "(?ix-m:ab+c)"
 */
static VALUE
og_oniguruma_oregexp_to_s(VALUE self)
{
  int options;
  VALUE str, og_mOniguruma;
  
  og_mOniguruma = rb_const_get(rb_cObject, rb_intern(OG_M_ONIGURUMA));
  
  options = FIX2INT(og_oniguruma_oregexp_options(self));
  str = rb_str_new2("(?");
  
  if ((options & FIX2INT(rb_const_get(og_mOniguruma, rb_intern("OPTION_IGNORECASE")))) > 0)
    rb_str_cat(str, "i", 1);

  if ((options & FIX2INT(rb_const_get(og_mOniguruma, rb_intern("OPTION_MULTILINE")))) > 0)
    rb_str_cat(str, "m", 1);

  if ((options & FIX2INT(rb_const_get(og_mOniguruma, rb_intern("OPTION_EXTEND")))) > 0)
    rb_str_cat(str, "x", 1);
  
  if (rb_str_cmp(str, rb_str_new2("(?imx")) != 0) {
    rb_str_cat(str, "-", 1);
    
    if ((options & FIX2INT(rb_const_get(og_mOniguruma, rb_intern("OPTION_IGNORECASE")))) == 0)
      rb_str_cat(str, "i", 1);

    if ((options & FIX2INT(rb_const_get(og_mOniguruma, rb_intern("OPTION_MULTILINE")))) == 0)
      rb_str_cat(str, "m", 1);

    if ((options & FIX2INT(rb_const_get(og_mOniguruma, rb_intern("OPTION_EXTEND")))) == 0)
      rb_str_cat(str, "x", 1);
  }
  
  rb_str_cat(str, ":", 1);
  rb_str_concat(str, rb_iv_get(self, "@pattern"));
  return rb_str_cat(str, ")", 1);
}

/*
 * Document-method: inspect
 *
 * call-seq:
 *    rxp.inspect   => string
 *
 * Returns a readable version of <i>rxp</i>
 *
 *    ORegexp.new( 'cat', :options => OPTION_MULTILINE | OPTION_IGNORECASE ).inspect  => /cat/im
 *    ORegexp.new( 'cat', :options => OPTION_MULTILINE | OPTION_IGNORECASE ).to_s     => (?im-x)cat
 */
static VALUE
og_oniguruma_oregexp_inspect(VALUE self)
{
  int options;
  VALUE str, og_mOniguruma;
  
  og_mOniguruma = rb_const_get(rb_cObject, rb_intern(OG_M_ONIGURUMA));
  
  options = FIX2INT(og_oniguruma_oregexp_options(self));
  str = rb_str_new2("/");
  rb_str_concat(str, rb_iv_get(self, "@pattern"));
  rb_str_cat(str, "/", 1);
  
  if ((options & FIX2INT(rb_const_get(og_mOniguruma, rb_intern("OPTION_IGNORECASE")))) > 0)
    rb_str_cat(str, "i", 1);

  if ((options & FIX2INT(rb_const_get(og_mOniguruma, rb_intern("OPTION_MULTILINE")))) > 0)
    rb_str_cat(str, "m", 1);

  if ((options & FIX2INT(rb_const_get(og_mOniguruma, rb_intern("OPTION_EXTEND")))) > 0)
    rb_str_cat(str, "x", 1);
  
  return str;
}

void
og_oniguruma_oregexp(VALUE mod, const char* name)
{
  VALUE og_cOniguruma_ORegexp, og_cOniguruma_ORegexp_Singleton;
  
  og_cOniguruma_ORegexp = rb_define_class_under(mod, name, rb_cObject);
  rb_define_alloc_func(og_cOniguruma_ORegexp, og_oniguruma_oregexp_alloc);
  
  /* Now add the methods to the class */
  rb_define_singleton_method(og_cOniguruma_ORegexp, "escape",     og_oniguruma_oregexp_escape,      -1);
  rb_define_singleton_method(og_cOniguruma_ORegexp, "last_match", og_oniguruma_oregexp_last_match,  -1);
  
  /* Define Instance Methods */
  rb_define_method(og_cOniguruma_ORegexp, "initialize", og_oniguruma_oregexp_initialize,            -1);
  rb_define_method(og_cOniguruma_ORegexp, "match",      og_oniguruma_oregexp_match,                 -1);
  rb_define_method(og_cOniguruma_ORegexp, "=~",         og_oniguruma_oregexp_operator_match,         1);
  rb_define_method(og_cOniguruma_ORegexp, "==",         og_oniguruma_oregexp_operator_equality,      1);
  rb_define_method(og_cOniguruma_ORegexp, "===",        og_oniguruma_oregexp_operator_identical,     1);
  rb_define_method(og_cOniguruma_ORegexp, "sub",        og_oniguruma_oregexp_sub,                   -1);
  rb_define_method(og_cOniguruma_ORegexp, "sub!",       og_oniguruma_oregexp_sub_bang,              -1);
  rb_define_method(og_cOniguruma_ORegexp, "gsub",       og_oniguruma_oregexp_gsub,                  -1);
  rb_define_method(og_cOniguruma_ORegexp, "gsub!",      og_oniguruma_oregexp_gsub_bang,             -1);
  rb_define_method(og_cOniguruma_ORegexp, "scan",       og_oniguruma_oregexp_scan,                   1);
  rb_define_method(og_cOniguruma_ORegexp, "casefold?",  og_oniguruma_oregexp_casefold,               0);
  rb_define_method(og_cOniguruma_ORegexp, "kcode",      og_oniguruma_oregexp_kcode,                  0);
  rb_define_method(og_cOniguruma_ORegexp, "options",    og_oniguruma_oregexp_options,                0);
  rb_define_method(og_cOniguruma_ORegexp, "source",     og_oniguruma_oregexp_source,                 0);
  rb_define_method(og_cOniguruma_ORegexp, "inspect",    og_oniguruma_oregexp_inspect,                0);
  rb_define_method(og_cOniguruma_ORegexp, "to_s",       og_oniguruma_oregexp_to_s,                   0);
  
  /* Define Aliases */
  /* Instance method aliases */
  rb_define_alias(og_cOniguruma_ORegexp, "eql?", "==");
  rb_define_alias(og_cOniguruma_ORegexp, "match_all", "scan");
  
  /* Class method aliases */
  og_cOniguruma_ORegexp_Singleton = rb_singleton_class(og_cOniguruma_ORegexp);
  rb_define_alias(og_cOniguruma_ORegexp_Singleton, "compile", "new");
  rb_define_alias(og_cOniguruma_ORegexp_Singleton, "quote", "escape");
}
