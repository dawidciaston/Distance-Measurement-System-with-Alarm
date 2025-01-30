

#ifndef FRDM_BSP_H
#define FRDM_BSP_H

/******************************************************************************\
* Global definitions
\******************************************************************************/
#define FRDM_KL05Z       										 /* define eval used in project */

#ifndef TRUE
# define TRUE        (1)
#endif

#ifndef	FALSE
# define FALSE       (0)
#endif

/******************************************************************************\
* Global macros
\******************************************************************************/
#define MASK(x)		 (uint32_t)(1UL << (x))    /* turn bit number into 32b mask */
#define DELAY(x)   for(uint32_t i=0;i<(x*1048);i++)__NOP(); 					/* wait */

#ifdef FRDM_KL05Z
# include "MKL05Z4.h"                        /* header with CMSIS */
#endif /* FRDM_KL05Z */

#endif /* FRDM_BSP_H */
