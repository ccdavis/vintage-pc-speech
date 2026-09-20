#ifndef _TYPES_H
#define _TYPES_H
/***************************************************************************
 *
 * TYPES.H
 *
 * Description - These are the base types for the whole system.
 *
 * Author      - Jonathan Hornstein
 *
 * Change Log
 *
 * Date      Initials   Description
 * 16-05-92  JSH        Created today because I like all types to be in upper
 *		   	            case
 *
 ***************************************************************************/
typedef char BOOL;

/***** Common #defines *****/
#define TRUE  				1
#define FALSE 				0

#define ON_D  				1
#define OFF_D 				0
#define FAIL_D				-1
#define SUCCEED_D           0
#define ERROR_D		        -1

/***** Common Macros   *****/
#define ARRAYSIZE_M(a)				(sizeof(a)/sizeof(a[0]))
#define OFFSET_OF_M(type, member)	&(((type)0)->member)
#define MAKETYPE_M(type, value)		*((type *)&value)
#define BIT_ON_M( A, B )			A=A|(1<<B)
#define BIT_OFF_M( A, B )			A=A&~(1<<B)
#define BIT_TEST_M( A, B )			(( A&(1<<B) )>>B)
#define TOGGLE_BIT_M( A, B )		A=A^( 1<<B )
#endif

