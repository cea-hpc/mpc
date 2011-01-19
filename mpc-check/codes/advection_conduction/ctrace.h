
/*
 * ctrace.h : interface du flux permettant la trace de l'exécution.
 *
 */

#ifndef UTILSPP_CTRACE_H
#define UTILSPP_CTRACE_H

#include <marcel_safe.h>
#if HAVE_PRAGMA_IDENT
#pragma ident "@(#)$Id: ctrace.h,v 1.1.1.1 2006/02/02 09:42:46 perache Exp $"
#endif

#include <iostream.h>

#define ctrace	cout

#define INFO_ERR "\nfichier : " << __FILE__ <<  " ligne : " << __LINE__ <<\
" "
#define INFO_ERR_TRACE "\nfichier : %s ligne : %d",  __FILE__, __LINE__

#endif

