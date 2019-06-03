#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define CONVERT	\
	"/usr/local/bin/convert -background black -fill red -flop -font font.ttf -pointsize 142 "

int main()
{
	int i;
	char buf[128];

	unlink ("out.c");

	for (i = 0x20; i < 256; i++) {
		unlink ("font/%02X.jpg");
		printf ("CHAR [%02X]\n", i);
		if (i == '\'')
			sprintf (buf, CONVERT "label:\"%c\" font/%02X.jpg\n", i, i);
		else if (i == '%')
			sprintf (buf, CONVERT "label:'%%' font/%02X.jpg\n", i);
		else if (i == '\\')
			sprintf (buf, CONVERT "label:'\\\\' font/%02X.jpg\n", i);
		else if (i == '@')
			sprintf (buf, CONVERT "label:'\\@' font/%02X.jpg\n", i);
		else if (i == ' ')
			sprintf (buf, CONVERT "label:' ' font/%02X.jpg\n", i);
		else
			sprintf (buf, CONVERT "label:'%c' font/%02X.jpg\n", i, i);
		/*printf ("CMD: [%s]\n", buf);*/
		system (buf);
	}
}
