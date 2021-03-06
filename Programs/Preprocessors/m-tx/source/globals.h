/* Header for module globals, generated by p2c 1.21alpha-07.Dec.93 */
#ifndef GLOBALS_H
#define GLOBALS_H


#ifdef GLOBALS_G
# define vextern
#else
# define vextern extern
#endif


/* 1. All global variables.
   2. Miscellaneous other procedures required by several Units.
*/

/* CMO: addition/change by Christian Mondrup */


#define PMXlinelength   128
/* !!! One or more of the following constants should be reduced if this
   program is to be compiled by a 16-bit compiler (e.g. Turbo Pascal),
   otherwise you get a "Data segment too large" error */
#define lines_in_paragraph  100
#define max_words       128
#define max_notes       128
/* Christian Mondrup's suggestion to reduce data segment size:
      lines_in_paragraph = 50;
      max_words = 64;
      max_notes = 64;
*/
#define max_bars        16
#define maxstaves       15
#define maxvoices       15
#define maxgroups       3
#define standardPMXvoices  12

#define max_lyrics_line_length  (PMXlinelength - 4)

#define inf             32000
#define unspec          1000
#define default_size    20

#define start_beam      '['
#define stop_beam       ']'
#define rest            'r'

#define pause           "rp"

#define dotcode         'd'
#define grace_group     'G'
#define multi_group     'x'
#define barsym          '|'
#define comment         '%'
#define blank           ' '
#define dot             '.'
#define comma           ','
#define colon           ':'
#define tilde           '~'
#define dummy           '\0'

#define ndurs           8

#define unspecified     '5'   /* Not a valid duration */

#define whole           2   /* position of '0' in durations */

#define digits          "123456789"
#define digitsdot       "0123456789."

#define putspace        true
#define nospace         false
#define print           true


typedef char paragraph_index;

typedef char voice_index;

typedef char stave_index;

typedef char bar_index0;

typedef uchar word_index0;

typedef char paragraph_index0;

typedef char voice_index0;

typedef char stave_index0;

typedef Char paragraph[lines_in_paragraph][256];
typedef short line_nos[lines_in_paragraph];


extern Char double_comment[3];
extern Char durations[ndurs + 1];
extern Char terminators[256];
extern Char has_duration[9];
extern Char solfa_names[8];

extern Char choice;
extern boolean outfile_open;
extern Char texdir[256];
extern Char old_meter_word[256];
extern short outlen;
extern boolean ignore_input;
vextern Char voice_label[maxvoices][256];
vextern Char clef[maxstaves];
vextern voice_index0 instr[maxstaves], stave[maxstaves],
		     first_on_stave[maxstaves], number_on_stave[maxstaves];
vextern short nspace[maxstaves], stave_size[maxstaves];
vextern voice_index0 nvoices, nstaves, ninstr, bottom, top;
vextern short one_beat, full_bar, line_no, short_note, musicsize, meternum,
	      meterdenom, pmnum, pmdenom, paragraph_no, bar_no, pickup, nbars,
	      nleft;
vextern paragraph_index0 para_len;
vextern double xmtrnum0;
vextern paragraph P, orig_P;
vextern line_nos orig_line_no;
vextern FILE *infile, *outfile, *stylefile;
vextern Char default_duration;
vextern Char fracindent[256], this_version[256], this_version_date[256];
vextern boolean pmx_preamble_done, first_paragraph, final_paragraph,
		must_respace, must_restyle, multi_bar_rest, some_vocal;
vextern Char infile_NAME[_FNSIZE];
vextern Char outfile_NAME[_FNSIZE];
vextern Char stylefile_NAME[_FNSIZE];


extern void error(Char *message, boolean printLine);
extern void fatalerror(Char *message);
extern void warning(Char *message, boolean printLine);
extern short PMXinstr(short stave);
extern void setDefaultDuration(short meterdenom);
extern void getMeter(Char *line, short *meternum, short *meterdenom,
		     short *pmnum, short *pmdenom);
extern void setSpace(Char *line);
extern Char *meterChange(Char *Result, short n1, short n2, boolean blind);
extern Char *meterWord(Char *Result, short num, short denom, short pnum,
		       short pdenom);
extern void cancel(short *num, short *denom, short lowest);
extern boolean isNoteOrRest(Char *w);
extern boolean isPause(Char *note);
/* CMO: */
extern short PMXmeterdenom(short denom);


#undef vextern

#endif /*GLOBALS_H*/

/* End. */
