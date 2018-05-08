#include "libstr.h"
#include "asar.h"
#include "autoarray.h"
#include "assocarr.h"
#include "errors.h"

bool confirmname(const char * name);

struct macrodata
{
autoarray<string> lines;
int numlines;
int startline;
const char * fname;
const char ** arguments;
int numargs;
};

assocarr<macrodata*> macros;
string thisname;
macrodata * thisone;
int numlines;

extern string thisfilename;
extern int thisline;
extern const char * thisblock;

extern autoarray<whiletracker> whilestatus;

void assembleline(const char * fname, int linenum, const char * line);

int reallycalledmacros;
int calledmacros;
int macrorecursion;

extern int repeatnext;
extern int numif;
extern int numtrue;

extern const char * callerfilename;
extern int callerline;

void startmacro(const char * line_)
{
	thisone=NULL;
	if (!confirmqpar(line_)) asar_throw_error(0, error_type_block, error_id_broken_macro_declaration);
	string line=line_;
	clean(line);
	char * startpar=strqchr(line.str, '(');
	if (!startpar) asar_throw_error(0, error_type_block, error_id_broken_macro_declaration);
	*startpar=0;
	startpar++;
	if (!confirmname(line)) asar_throw_error(0, error_type_block, error_id_invalid_macro_name);
	thisname=line;
	char * endpar=strqrchr(startpar, ')');
	//confirmqpar requires that all parentheses are matched, and a starting one exists, therefore it is harmless to not check for nulls
	if (endpar[1]) asar_throw_error(0, error_type_block, error_id_broken_macro_declaration);
	*endpar=0;
	for (int i=0;startpar[i];i++)
	{
		char c=startpar[i];
		if (!isalnum(c) && c!='_' && c!=',') asar_throw_error(0, error_type_block, error_id_broken_macro_declaration);
		if (c==',' && isdigit(startpar[i+1])) asar_throw_error(0, error_type_block, error_id_broken_macro_declaration);
	}
	if (*startpar==',' || isdigit(*startpar) || strstr(startpar, ",,") || endpar[-1]==',') asar_throw_error(0, error_type_block, error_id_broken_macro_declaration);
	if (macros.exists(thisname)) asar_throw_error(0, error_type_block, error_id_macro_redefined, thisname.str);
	thisone=(macrodata*)malloc(sizeof(macrodata));
	new(thisone) macrodata;
	if (*startpar)
	{
		thisone->arguments=(const char**)qpsplit(strdup(startpar), ",", &thisone->numargs);
	}
	else
	{
		const char ** noargs=(const char**)malloc(sizeof(const char**));
		*noargs=NULL;
		thisone->arguments=noargs;
		thisone->numargs=0;
	}
	for (int i=0;thisone->arguments[i];i++)
	{
		if (!confirmname(thisone->arguments[i])) asar_throw_error(0, error_type_block, error_id_invalid_macro_param_name);
		for (int j=i+1;thisone->arguments[j];j++)
		{
			if (!strcmp(thisone->arguments[i], thisone->arguments[j])) asar_throw_error(0, error_type_block, error_id_macro_param_redefined, thisone->arguments[i]);
		}
	}
	thisone->fname=strdup(thisfilename);
	thisone->startline=thisline;
	numlines=0;
}

void tomacro(const char * line)
{
	if (!thisone) return;
	thisone->lines[numlines++]=line;
}

void endmacro(bool insert)
{
	if (!thisone) return;
	thisone->numlines=numlines;
	if (insert) macros.create(thisname) = thisone;
	else delete thisone;
}


extern autoarray<int>* macroposlabels;
extern autoarray<int>* macroneglabels;
extern autoarray<string>* macrosublabels;

void callmacro(const char * data)
{
	int numcm=reallycalledmacros++;
	macrodata * thismacro;
	if (!confirmqpar(data)) asar_throw_error(0, error_type_block, error_id_broken_macro_usage);
	string line=data;
	clean(line);
	char * startpar=strqchr(line.str, '(');
	if (!startpar) asar_throw_error(0, error_type_block, error_id_broken_macro_usage);
	*startpar=0;
	startpar++;
	if (!confirmname(line)) asar_throw_error(0, error_type_block, error_id_broken_macro_usage);
	if (!macros.exists(line)) asar_throw_error(0, error_type_block, error_id_macro_not_found, line.str);
	thismacro = macros.find(line);
	char * endpar=strqrchr(startpar, ')');
	if (endpar[1]) asar_throw_error(0, error_type_block, error_id_broken_macro_usage);
	*endpar=0;
	autoptr<const char **> args;
	int numargs=0;
	if (*startpar) args=(const char**)qpsplit(strdup(startpar), ",", &numargs);
	if (numargs != thismacro->numargs) asar_throw_error(0, error_type_block, error_id_macro_wrong_num_params);
	macrorecursion++;
	int startif=numif;

	autoarray<int>* oldmacroposlabels = macroposlabels;
	autoarray<int>* oldmacroneglabels = macroneglabels;
	autoarray<string>* oldmacrosublabels = macrosublabels;

	autoarray<int> newmacroposlabels;
	autoarray<int> newmacroneglabels;
	autoarray<string> newmacrosublabels;

	macroposlabels = &newmacroposlabels;
	macroneglabels = &newmacroneglabels;
	macrosublabels = &newmacrosublabels;

	for (int i=0;i<thismacro->numlines;i++)
	{
		try
		{
			thisfilename= thismacro->fname;
			thisline= thismacro->startline+i+1;
			thisblock=NULL;
			string out;
			string connectedline;
			int skiplines = getconnectedlines<autoarray<string> >(thismacro->lines, i, connectedline);
			string intmp = connectedline;
			for (char * in=intmp.str;*in;)
			{
				if (*in=='<' && in[1]=='<')
				{
					out+="<<";
					in+=2;
				}
				else if (*in=='<' && isalnum(in[1]))
				{
					char * end=in+1;
					while (*end && *end!='<' && *end!='>') end++;
					if (*end!='>')
					{
						out+=*(in++);
						continue;
					}
					*end=0;
					in++;
					if (!confirmname(in)) asar_throw_error(0, error_type_block, error_id_broken_macro_contents);
					bool found=false;
					for (int j=0;thismacro->arguments[j];j++)
					{
						if (!strcmp(in, thismacro->arguments[j]))
						{
							found=true;
							if (args[j][0]=='"')
							{
								string s=args[j];
								out+=dequote(s.str);
							}
							else out+=args[j];
							break;
						}
					}
					if (!found) asar_throw_error(0, error_type_block, error_id_macro_param_not_found, in);
					in=end+1;
				}
				else out+=*(in++);
			}
			calledmacros = numcm;
			int prevnumif = numif;
			assembleline(thismacro->fname, thismacro->startline+i, out);
			i += skiplines;
			if (numif != prevnumif && whilestatus[numif].iswhile && whilestatus[numif].cond)
				i = whilestatus[numif].startline - thismacro->startline - 1;
		}
		catch(errline&){}
	}

	macroposlabels = oldmacroposlabels;
	macroneglabels = oldmacroneglabels;
	macrosublabels = oldmacrosublabels;

	macrorecursion--;
	if (repeatnext!=1)
	{
		thisblock=NULL;
		repeatnext=1;
		asar_throw_error(0, error_type_block, error_id_rep_at_macro_end);
	}
	if (numif!=startif)
	{
		thisblock=NULL;
		numif=startif;
		numtrue=startif;
		asar_throw_error(0, error_type_block, error_id_unclosed_if);
	}
}
