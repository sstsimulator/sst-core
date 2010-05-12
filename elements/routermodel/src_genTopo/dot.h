/*
** $Id: dot.h,v 1.3 2010/04/27 19:48:31 rolf Exp $
**
** Rolf Riesen, March 2010, Sandia National Laboratories
**
*/
#ifndef _DOT_H_
#define _DOT_H_

#include <stdio.h>


void dot_header(FILE *dotfile, char *graph_name);
void dot_body(FILE *dotfile);
void dot_footer(FILE *dotfile);

#endif /* _DOT_H_ */
