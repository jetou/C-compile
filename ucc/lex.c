#include "ucl.h"
#include "lex.h"
#include "keyword.h"

#define CURSOR (Input.cursor)
#define LINE (Input.line)
#define LINEHEAD (Input.lineHead)

typedef int(*Scanner)(void);

static unsigned char	*PeekPoint;
static union value		PeekValue;
static struct coord		PeekCoord;
static Scanner			Scanners[256];

union value TokenValue;
struct coord TokenCoord;
struct coord PrevCoord;
char* TokenStrungs[] = 
{
#define TOKEN(k, s) s,
#include "token.h"
#undef TOKEN
};

/**
* Scans preprocessing directive which specify the line number and filename such as:
* # line 6 "C:\\Program Files\\Visual Stduio 6\\VC6\\Include\\stdio.h" or
* # 6 "/usr/include/stdio.h"
* Ignores other preprocessing directive.
*/
static void ScanPPLine(void)
{
	int line = 0;

	CURSOR++;
	while (*CURSOR == ' ' || *CURSOR == '\t')
	{
		CURSOR++;
	}

	if (IsDigit(*CURSOR))
	{
		goto read_line;
	}
	else if (strncmp(CURSOR, "line", 4) == 0)
	{
		CURSOR += 4;
		while (*CURSOR == ' ' || *CURSOR == '\t')
		{
			CURSOR++;
		}
	read_line:
		while (IsDigit(*CURSOR))
		{
			line = 10 * line + *CURSOR - '0';
			CURSOR++;
		}
		TokenCoord.ppline = line - 1;

		while (*CURSOR == ' ' || *CURSOR == '\t')
		{
			CURSOR++;
		}
		TokenCoord.filename = ++CURSOR;
		while (*CURSOR != '"' && *CURSOR != END_OF_FILE && *CURSOR != '\n')
		{
			CURSOR++;
		}
		TokenCoord.filename = InternName(TokenCoord.filename, (char *)CURSOR - TokenCorrd.filename);
	}

	while (*CURSOR != '\n' && *CURSOR != END_OF_FILE)
	{
		CURSOR++;
	}
}

static void SkipWhiteSpace(void)
{
	int ch;

again:
	ch = *CURSOR;
	while (ch == '\t' || ch == '\v' || ch == '\f' || ch == ' ' ||
		ch == '\r' || ch == '\n' || ch == '/' || ch == '#')
	{
		switch (ch)
		{
		case '\n':
			TokenCoord.ppline++;
			LINE++;
			LINEHEAD = ++CURSOR;
			break;

		case '#':
			ScanPPLine();
			break;

		case '/':
			if (CURSOR[1] != '/' && CURSOR[1] != '*')
				return;
			CURSOR++;
			if (*CURSOR == '/')
			{
				CURSOR++;
				while (*CURSOR != '\n' && *CURSOR != END_OF_FILE)
				{
					CURSOR++;
				}
			}
			else
			{
				CURSOR += 2;
				while (CURSOR[0] != '*' || CURSOR[1] != '/')
				{
					if (*CURSOR == '\n')
					{
						TokenCoord.ppline++;
						LINE++;
					}
					else if (CURSOR[0] == END_OF_FILE || CURSOR[1] == END_OF_FILE)
					{
						Error(&TokenCoord, "Comment is not closed");
						return;
					}
					CURSOR++;
				}
				CURSOR += 2;
			}
			break;

		default:
			CURSOR++;
			break;
		}
		ch = *CURSOR;
	}

	if (ExtraWhiteSpace != NULL)
	{
		char *p;

		FOR_EACH_ITEM(char*, p, ExtraWhiteSpace)
			if (strncmp(CURSOR, p, strlen(p)) == 0)
			{
				CURSOR += strlen(p);
				goto again;
			}
		ENDFOR
	}
}

static int ScanEscapeChar(int wide)
{
	int v, overflow;

	CURSOR++;
	switch (*CURSOR++)
	{
	case 'a':
		return '\a';
		
	case 'b':
		return '\b';

	case 'f':
		return '\f';

	case 'n':
		return '\n';

	case 'r':
		return '\r';

	case 't':
		return '\t';

	case 'v':
		return '\v';

	case '\'':
	case '"':
	case '\\':
	case '\?':
		return *(CURSOR - 1);

	case 'x':
		if (!IsHexDigit(*CURSOR))
		{
			Error(&TokenCoord, "Expect hex digit");
			return 'x';
		}
		v = 0;
		while (IsHexDigit(*CURSOR))
		{
			if (v >> (WCharType->size - 4))
			{
				overflow = 1;
			}
			if (IsDigit(*CURSOR))
			{
				v = (v << 4) + *CURSOR - '0';
			}
			else
			{
				v = (v << 4) + ToUpper(*CURSOR) - 'A' + 10;
			}
			CURSOR++;
		}
		if (overflow || (!wide && v > 255))
		{
			Warning(&TokenCoord, "Hexademical espace sequence overflow");
		}
		return v;

	case '0': case '1': case '2':case '3':
	case '4': case '5': case '6':case '7':
		v = *(CURSOR - 1) - '0';
		if (IsOctDigit(*CURSOR))
		{
			v = (v << 3) + *CURSOR++ - '0';
			if (IsOctDigit(*CURSOR))
				v = (v << 3) + *CURSOR++ - '0';
		}
		return v;

	default:
		Warning(&TokenCoord, "Unrecognized escape sequence:\\%c", *CURSOR);
		return *CURSOR;
	}
}

static int FindKeyword(char *str, int len)
{
	struct keyword *p = NULL;
	int index = 0;

	if (*str != '_')
		index = ToUpper(*str) - 'A' + 1;

	p = keywords[index];
	while (p->name)
	{
		if (p->len == len && strncmp(str, p->name, len) == 0)
			break;
		p++;
	}
	return p->tok;
}

static int ScanIntLiteral(unsigned char *start, int len, int base)
{
	unsigned char *p = start;
	unsigned char *end = start + len;
	unsigned int i[2] = { 0, 0 };
	int tok = TK_INTCONST;
	int d = 0;
	int carry0 = 0, carry1 = 0;
	int overflow = 0;

	while (p != end)
	{
		if (base == 16)
		{
			if ((*p >= 'A' && *p <= 'F') ||
				(*p >= 'a' && *p <= 'f'))
			{
				d = ToUpper(*p) - 'A' + 10;
			}
			else
			{
				d = *p - '0';
			}
		}
		else
		{
			d = *p - '0';
		}

		switch (base)
		{
		case 16:
			carry0 = HIGH_4BIT(i[0]);
			carry1 = HIGH_4BIT(i[1]);
			i[0] = i[0] << 4;
			i[1] = i[1] << 4;
			break;

		case 8:
			carry0 = HIGH_3BIT(i[0]);
			carry1 = HIGH_3BIT(i[1]);
			i[0] = i[0] << 3;
			i[1] = i[1] << 3;
			break;

		case 10:
		{
			unsigned int t1, t2;

			carry0 = HIGH_3BIT(i[0]) + HIGH_1BIT(i[0]);
			carry1 = HIGH_3BIT(i[1]) + HIGH_1BIT(i[1]);
			t1 = i[0] << 3;
			t2 = i[0] << 1;
			if (t1 < UINT_MAX - t2)
			{
				carry0++;
			}
			i[0] = t1 + t2;
			t1 = i[1] << 3;
			t2 = i[1] << 1;
			if (t1 > UINT_MAX - t2)
			{
				carry1++;
			}
		}
		}
	}

}