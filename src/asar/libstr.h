#pragma once

#include "std-includes.h"

inline char *copy(const char *source, int copy_length, char *dest)
{
	memmove(dest, source, copy_length*sizeof(char));
	return dest;
}

inline int min(int a, int b)
{
	return a > b ? b : a;
}

inline int bit_round(int v)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

class string {
public:
const char *data() const
{
	return is_inlined() ? inlined.str : allocated.str;
}

char *temp_raw() const	//things to cleanup and take a look at
{
	return is_inlined() ? (char *)inlined.str : allocated.str;
}

char *raw() const
{
	return is_inlined() ? (char *)inlined.str : allocated.str;
}

int length() const
{
	return is_inlined() ? inlined.len : allocated.len;
}

void set_length(int length)
{
	if(length > max_inline_length_){
		inlined.len = (unsigned char)-1;
		allocated.len = length;
	}else{
		inlined.len = length;
	}
}

void assign(const char * newstr)
{
	if (!newstr) newstr = "";	//todo is this proper?
	assign(newstr, strlen(newstr));
}

void assign(const char * newstr, int end)
{
	resize(end);
	copy(newstr, length(), raw());
}


string& operator=(const char * newstr)
{
	assign(newstr);
	return *this;
}

string& operator=(string newstr)
{
	assign(newstr);
	return *this;
}

string& operator+=(const string& other)
{
	int current_end = length();
	resize(length() + other.length());
	copy(other.data(), other.length(), raw() + current_end);
	return *this;
}

string& operator+=(const char *other)
{
	int current_end = length();
	int otherlen=(int)strlen(other);
	resize(length() + otherlen);
	copy(other, otherlen, raw() + current_end);
	return *this;
}

string& operator+=(char c)
{
	resize(length() + 1);
	raw()[length() - 1] = c;
	return *this;
}

string operator+(char right) const
{
	string ret=*this;
	ret+=right;
	return ret;
}

string operator+(const char * right) const
{
	string ret=*this;
	ret+=right;
	return ret;
}

bool operator==(const char * right) const
{
	return !strcmp(data(), right);
}

bool operator==(string& right) const
{
	return !strcmp(data(), right.data());
}

bool operator!=(const char * right) const
{
	return (strcmp(data(), right) != 0);
}

bool operator!=(string& right) const
{
	return (strcmp(data(), right.data()) != 0);
}

operator const char*() const
{
	return data();
}

explicit operator bool() const
{
	return length();
}

string()
{
	//todo reduce I know this isn't all needed
	allocated.bufferlen = 0;
	allocated.str = 0;
	allocated.len = 0;
	inlined.len = 0;

}
string(const char * newstr) : string()
{
	assign(newstr);
}
string(const char * newstr, int newlen) : string()
{
	assign(newstr, newlen);
}
string(const string& old) : string()
{
	assign(old.data());
}
/*
string(string &&move)
{
	if(!move.is_inlined()){
		allocated.str = move.allocated.str;
		allocated.len = move.allocated.len;
		allocated.bufferlen = move.allocated.bufferlen;
		move.allocated.str = nullptr;
		move.set_length(0);
	}else{
		inlined.len = move.inlined.len;
		copy(move.inlined.str, inlined.len, inlined.str);
	}
}
*/
~string()
{
	if(!is_inlined()){
		free(allocated.str);
	}
}

string& replace(const char * instr, const char * outstr, bool all=true);
string& qreplace(const char * instr, const char * outstr, bool all=true);

#ifdef SERIALIZER
void serialize(serializer & s)
{
	s(str, allocated.bufferlen);
	set_length(strlen(str));
}
#endif
#define SERIALIZER_BANNED

private:
static const int scale_factor = 3; //scale sso
static const int max_inline_length_ = ((sizeof(char *) + sizeof(int) * 2) * scale_factor) - 2;
union{
	struct{
		char str[max_inline_length_ + 1];
		unsigned char len = 0;
	}inlined;
	struct{
		char *str;
		int len;
		int bufferlen;
	}allocated;
};
		

void resize(int new_length)
{
	const char *old_data = data();
	if(new_length > max_inline_length_ && (is_inlined() || allocated.bufferlen <= new_length)){ //SSO or big to big
		int new_size = bit_round(new_length + 1);
		if(old_data == inlined.str){
			allocated.str = copy(old_data, min(length(), new_length), (char *)malloc(new_size));
		}else{
			allocated.str = (char *)realloc(allocated.str, new_size);
			old_data = inlined.str;	//this will prevent freeing a dead realloc ptr
		}
		allocated.bufferlen = new_size;
	}else if(length() >= max_inline_length_ && new_length < max_inline_length_){ //big to SSO
		copy(old_data, new_length, inlined.str);
	}
	set_length(new_length);
	
	if(old_data != inlined.str && old_data != data()){
		free((char *)old_data);
	}
	raw()[new_length] = 0; //always ensure null terminator
}

bool is_inlined() const
{
	return inlined.len != (unsigned char)-1;
}
};
#define S (string)

char * readfile(const char * fname, const char * basepath);
char * readfilenative(const char * fname);
bool readfile(const char * fname, const char * basepath, char ** data, int * len);//if you want an uchar*, cast it
char ** nsplit(char * str, const char * key, int maxlen, int * len);
char ** qnsplit(char * str, const char * key, int maxlen, int * len);
char ** qpnsplit(char * str, const char * key, int maxlen, int * len);
inline char ** split(char * str, const char * key, int * len= nullptr) { return nsplit(str, key, 0, len); }
inline char ** qsplit(char * str, const char * key, int * len= nullptr) { return qnsplit(str, key, 0, len); }
inline char ** qpsplit(char * str, const char * key, int * len= nullptr) { return qpnsplit(str, key, 0, len); }
inline char ** split1(char * str, const char * key, int * len= nullptr) { return nsplit(str, key, 2, len); }
inline char ** qsplit1(char * str, const char * key, int * len= nullptr) { return qnsplit(str, key, 2, len); }
inline char ** qpsplit1(char * str, const char * key, int * len= nullptr) { return qpnsplit(str, key, 2, len); }
//void replace(string& str, const char * instr, const char * outstr, bool all);
//void qreplace(string& str, const char * instr, const char * outstr, bool all);
bool confirmquotes(const char * str);
bool confirmqpar(const char * str);
char* strqpchr(const char* str, char key);

inline string hex(unsigned int value)
{
	char buffer[64];
	if(0);
	else if (value<=0x000000FF) sprintf(buffer, "%.2X", value);
	else if (value<=0x0000FFFF) sprintf(buffer, "%.4X", value);
	else if (value<=0x00FFFFFF) sprintf(buffer, "%.6X", value);
	else sprintf(buffer, "%.8X", value);
	return buffer;
}

inline string hex(unsigned int value, int width)
{
	char buffer[64];
	sprintf(buffer, "%.*X", width, value);
	return buffer;
}

inline string hex0(unsigned int value)
{
	char buffer[64];
	sprintf(buffer, "%X", value);
	return buffer;
}

inline string hex2(unsigned int value)
{
	char buffer[64];
	sprintf(buffer, "%.2X", value);
	return buffer;
}

inline string hex3(unsigned int value)
{
	char buffer[64];
	sprintf(buffer, "%.3X", value);
	return buffer;
}

inline string hex4(unsigned int value)
{
	char buffer[64];
	sprintf(buffer, "%.4X", value);
	return buffer;
}

inline string hex5(unsigned int value)
{
	char buffer[64];
	sprintf(buffer, "%.5X", value);
	return buffer;
}

inline string hex6(unsigned int value)
{
	char buffer[64];
	sprintf(buffer, "%.6X", value);
	return buffer;
}

inline string hex8(unsigned int value)
{
	char buffer[64];
	sprintf(buffer, "%.8X", value);
	return buffer;
}

inline string dec(int value)
{
	char buffer[64];
	sprintf(buffer, "%i", value);
	return buffer;
}

inline string ftostr(double value)
{
	// randomdude999: With 100 digits of precision, the buffer needs to be approx. 311+100,
	// but let's be safe here https://stackoverflow.com/questions/7235456
	char rval[512];
	// RPG Hacker: Ridiculously high precision, I know, but we're working with doubles
	// here and can afford it, so no need to waste any precision
	sprintf(rval, "%.100f", value);
	if (strchr(rval, '.'))//nuke useless zeroes
	{
		char * end=strrchr(rval, '\0')-1;
		while (*end=='0')
		{
			*end='\0';
			end--;
		}
		if (*end=='.') *end='\0';
	}
	return rval;
}

// Same as above, but with variable precision
inline string ftostrvar(double value, int precision)
{
	int clampedprecision = precision;
	if (clampedprecision < 0) clampedprecision = 0;
	if (clampedprecision > 100) clampedprecision = 100;
	
	// see above
	char rval[512];
	sprintf(rval, "%.*f", clampedprecision, (double)value);
	if (strchr(rval, '.'))//nuke useless zeroes
	{
		char * end = strrchr(rval, '\0') - 1;
		while (*end == '0')
		{
			*end = '\0';
			end--;
		}
		if (*end == '.') *end = '\0';
	}
	return rval;
}

inline bool stribegin(const char * str, const char * key)
{
	for (int i=0;key[i];i++)
	{
		if (tolower(str[i])!=tolower(key[i])) return false;
	}
	return true;
}

inline bool striend(const char * str, const char * key)
{
	const char * keyend=strrchr(key, '\0');
	const char * strend=strrchr(str, '\0');
	while (key!=keyend)
	{
		keyend--;
		strend--;
		if (tolower(*strend)!=tolower(*keyend)) return false;
	}
	return true;
}

//function: return the string without quotes around it, if any exists
//if they don't exist, return it unaltered
//it is not guaranteed to return str
//it is not guaranteed to not edit str
//the input must be freed even though it's garbage, the output must not
inline const char * dequote(char * str)
{
	if (*str!='"') return str;
	int inpos=1;
	int outpos=0;
	while (true)
	{
		if (str[inpos]=='"')
		{
			if (str[inpos+1]=='"') inpos++;
			else if (str[inpos+1]=='\0') break;
			else return nullptr;
		}
		if (!str[inpos]) return nullptr;
		str[outpos++]=str[inpos++];
	}
	str[outpos]=0;
	return str;
}

inline char * strqchr(const char * str, char key)
{
	while (*str)
	{
		if (*str=='"')
		{
			str++;
			while (*str!='"')
			{
				if (!*str) return nullptr;
				str++;
			}
			str++;
		}
		else
		{
			if (*str==key) return const_cast<char*>(str);
			str++;
		}
	}
	return nullptr;
}

// RPG Hacker: WOW, these functions are poopy!
inline char * strqrchr(const char * str, char key)
{
	const char * ret= nullptr;
	while (*str)
	{
		if (*str=='"')
		{
			str++;
			while (*str!='"')
			{
				if (!*str) return nullptr;
				str++;
			}
			str++;
		}
		else
		{
			if (*str==key) ret=str;
			str++;
		}
	}
	return const_cast<char*>(ret);
}

inline string substr(const char * str, int len)
{
	return string(str, len);
}

char * itrim(char * str, const char * left, const char * right, bool multi=false);
string &itrim(string &str, const char * left, const char * right, bool multi=false);

inline string upper(const char * old)
{
	string s=old;
	for (int i=0;i<s.length();i++) s.raw()[i]=(char)toupper(s.data()[i]);
	return s;
}

inline string lower(const char * old)
{
	string s=old;
	for (int i=0;i<s.length();i++) s.raw()[i]=(char)tolower(s.data()[i]);
	return s;
}

inline int isualnum ( int c )
{
	return (c >= -1 && c <= 255 && (c=='_' || isalnum(c)));
}

inline int isualpha ( int c )
{
	return (c >= -1 && c <= 255 && (c=='_' || isalpha(c)));
}

#define ctype(type) \
		inline bool ctype_##type(const char * str) \
		{ \
			while (*str) \
			{ \
				if (!is##type(*str)) return false; \
				str++; \
			} \
			return true; \
		} \
		\
		inline bool ctype_##type(const char * str, int num) \
		{ \
			for (int i=0;i<num;i++) \
			{ \
				if (!is##type(str[i])) return false; \
			} \
			return true; \
		}
		
ctype(alnum)
ctype(alpha)
ctype(cntrl)
ctype(digit)
ctype(graph)
ctype(lower)
ctype(print)
ctype(punct)
ctype(space)
ctype(upper)
ctype(xdigit)
ctype(ualnum)
ctype(ualpha)
#undef ctype

inline const char * stristr(const char * string, const char * pattern)
{
	if (!*pattern) return string;
	const char * pptr;
	const char * sptr;
	const char * start;
	for (start=string;*start!=0;start++)
	{
		for (;(*start && (toupper(*start)!=toupper(*pattern)));start++);
		if (!*start) return nullptr;
		pptr=pattern;
		sptr=start;
		while (toupper(*sptr)==toupper(*pptr))
		{
			sptr++;
			pptr++;
			if (!*pptr) return start;
		}
	}
	return nullptr;
}



// Returns number of connected lines - 1
template<typename stringarraytype>
inline int getconnectedlines(stringarraytype& lines, int startline, string& out)
{
	out = string("");
	int count = 1;

	for (int i = startline; lines[i]; i++)
	{
		// The line should already be stripped of any comments at this point
		int linestartpos = (int)strlen(lines[i]);

		bool found = false;

		for (int j = linestartpos; j > 0; j--)
		{
			if (!isspace(lines[i][j]) && lines[i][j] != '\0' && lines[i][j] != ';')
			{
				if (lines[i][j] == '\\')
				{
					count++;
					out += string(lines[i], j);
					found = true;
					break;
				}
				else
				{
					out += string(lines[i], j + 1);
					return count - 1;
				}
			}
		}

		if (!found)
		{
			out += string(lines[i], 1);
			return count - 1;
		}
	}

	return count - 1;
}
