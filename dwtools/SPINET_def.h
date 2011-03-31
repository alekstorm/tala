/* SPINET_def.h */
/* David Weenink, 6 apr 1997 */

#define ooSTRUCT SPINET
oo_DEFINE_CLASS (SPINET, Sampled2)

	oo_LONG (gamma)						/* filter order */
	oo_DOUBLE (excitationErbProportion)	/* excitatory bandwidth proportionality factor*/
	oo_DOUBLE (inhibitionErbProportion)	/* inhibitatory bandwidth proportionality factor*/
	oo_DOUBLE_MATRIX (y, my ny, my nx) /* short term average energy spectrum */
	/* spectrum after on-center/off-surround and rectification */
	oo_DOUBLE_MATRIX (s, my ny, my nx)
		
oo_END_CLASS (SPINET)	
#undef ooSTRUCT

/* End of file SPINET_def.h */	
