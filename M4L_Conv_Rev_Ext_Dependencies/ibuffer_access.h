
/*
 *  ibuffer_access.h
 *
 *	This header file provides an interface for accessing and interpolating samplesfrom an ibuffer (or standard MSP buffer).
 *	You should also compile  ibufffer_access.c in the project
 *
 *	Various kinds of interpolation are supported.
 *	All pointers used should be 16-byte aligned.
 *
 *	The init routine should be called in an objects main function to setup the necessary variables.
 *	Use the ibuffer__get_ptr / ibuffer_info / ibuffer_sample_rate functions to get info about the ibuffer (or buffer).
 *	The buffer inuse field should be incremented and decremented beoore and after use using ibuffer_increment_inuse and ibuffer_decrement_inuse.
 *
 *	Integer offsets (representing sample position) should be preprocessed using ibuffer_preprocess_offsets before use (this will potentially change the stored values).
 *
 *	The main routines for accessing samples are then:
 *
 *	ibuffer_get_samp						- get a single sample
 *	ibuffer_get_samps						- get a number of consecutive samples
 *	ibuffer_get_samps_rev					- get a number of consecutive samples in reverse order
 *	inbuffer_samps_nointerp					- get samples based on offsets (no interpolation)
 *	ibuffer_calc_samps_lin					- calculate samples using linear interpolation
 *	ibuffer_calc_samps_cubic_bspline		- calculate samples using cubic bspline interpolation
 *	ibuffer_calc_samps_cubic_hermite		- calculate samples using cubic hermite interpolation
 *	ibuffer_calc_samps_cubic_lagrange		- calculate samples using cubic lagrange interpolation
 *
 *	offsets is a pointer to the sample offsets
 *	fracts is a pointer to the subsample (fractional) offsets
 *	temp is a pointer to an array of non-aliasing 16-byte aligned pointers each of which is the same size as the required output.
 *		- the array should be two pointers wide for linear interpolation and four for cubic interpolation
 *	n_samps is the number of samples required.
 *	n_chans is the number of channels in the buffer (as returned by a call the ibuffer_info).
 *	chan is the channel you wish to access.
 *	format is the format of the buffer (as returned by a call the ibuffer_info).
 *
 *	Most of these routines have a mul input to multiply by a constant value.
 *	Other variables should be self-explanatory 
 *	
 *  Copyright 2010 Alex Harker. All rights reserved.
 *
 */


#ifndef _IBUFFER_ACCESS_
#define _IBUFFER_ACCESS_

#include "ibuffer.h"
#include "AH_VectorOps.h"

t_symbol *ps_none;
t_symbol *ps_linear;
t_symbol *ps_bspline;
t_symbol *ps_hermite;
t_symbol *ps_lagrange;


enum {
	
	INTERP_TYPE_NONE,
	INTERP_TYPE_LIN,
	INTERP_TYPE_CUBIC_BSPLINE,
	INTERP_TYPE_CUBIC_HERMITE,
	INTERP_TYPE_CUBIC_LAGRANGE
	
};

// Call in main routine to initialise buffer symbols

void ibuffer_init ();

#ifdef __APPLE__

// Get ibuffer and related info (note that the sample rate is in a separate call, as it is not required info for other routines

static __inline void *ibuffer_get_ptr (t_symbol *s) FORCE_INLINE;
static __inline long ibuffer_info (void *thebuffer, void **samples, AH_SIntPtr *length, long *channels, long *format) FORCE_INLINE;
static __inline double ibuffer_sample_rate (void *thebuffer) FORCE_INLINE;

// Increment / decrement buffer inuse pointers

static __inline void ibuffer_increment_inuse (void *thebuffer) FORCE_INLINE;
static __inline void ibuffer_decrement_inuse (void *thebuffer) FORCE_INLINE;

// Get the value of an individual sample

static __inline float ibuffer_get_samp (void *samps, AH_SIntPtr offset, long n_chans, long chan, long format)  FORCE_INLINE;

// Calculate an offset to the samps pointer (accounting for sample interleaving and sample format)

static __inline void *ibuffer_offset (void *samps, AH_SIntPtr offset, long n_chans, long format) FORCE_INLINE;

// Process a set of offsets for use with the below routines (accounting for sample interleaving and sample format)

static __inline void ibuffer_preprocess_offsets (AH_SIntPtr *offsets, AH_SIntPtr n_samps, long n_chans, long format) FORCE_INLINE;

#endif

// Convert an array of 32 bit int format samples to floats with the correct scaling

__inline void convert_and_scale_int32_to_float (float *out, AH_SIntPtr n_samps);

// Get consecutive samples

void ibuffer_get_samps_16 (void *samps, float *out, AH_SIntPtr offset, AH_SIntPtr n_samps, long n_chans, long chan);
void ibuffer_get_samps_24 (void *samps, float *out, AH_SIntPtr offset, AH_SIntPtr n_samps, long n_chans, long chan);
void ibuffer_get_samps_32 (void *samps, float *out, AH_SIntPtr offset, AH_SIntPtr n_samps, long n_chans, long chan);
void ibuffer_get_samps_float (void *samps, float *out, AH_SIntPtr offset, AH_SIntPtr n_samps, long n_chans, long chan);

void ibuffer_get_samps (void *samps, float *out, AH_SIntPtr offset, AH_SIntPtr n_samps, long n_chans, long chan, long format);

// Get consecutive samples in reverse

void ibuffer_get_samps_rev_16 (void *samps, float *out, AH_SIntPtr offset, AH_SIntPtr n_samps, long n_chans, long chan);
void ibuffer_get_samps_rev_24 (void *samps, float *out, AH_SIntPtr offset, AH_SIntPtr n_samps, long n_chans, long chan);
void ibuffer_get_samps_rev_32 (void *samps, float *out, AH_SIntPtr offset, AH_SIntPtr n_samps, long n_chans, long chan);
void ibuffer_get_samps_rev_float (void *samps, float *out, AH_SIntPtr offset, AH_SIntPtr n_samps, long n_chans, long chan);

void ibuffer_get_samps_rev (void *samps, float *out, AH_SIntPtr offset, AH_SIntPtr n_samps, long n_chans, long chan, long format);

// Fetch samples from offsets as either 32 bit float or 32 bit int

__inline void ibuffer_fetch_samps_float (float *out, float *samps, AH_SIntPtr *offsets, AH_SIntPtr n_samps);
__inline void ibuffer_fetch_samps_16 (AH_SInt32 *out, AH_SInt16 *samps, AH_SIntPtr *offsets, AH_SIntPtr n_samps);
__inline void ibuffer_fetch_samps_24 (AH_SInt32 *out, AH_SInt8 *samps, AH_SIntPtr *offsets, AH_SIntPtr n_samps);
__inline void ibuffer_fetch_samps_32 (AH_SInt32 *out, AH_SInt32 *samps, AH_SIntPtr *offsets, AH_SIntPtr n_samps);

// No interpolation

void ibuffer_calc_samps_no_interp_16  (void *samps, float *out, AH_SIntPtr *offsets, AH_SIntPtr n_samps, long n_chans, long chan, float mul);
void ibuffer_calc_samps_no_interp_24  (void *samps, float *out, AH_SIntPtr *offsets, AH_SIntPtr n_samps, long n_chans, long chan, float mul);
void ibuffer_calc_samps_no_interp_32 (void *samps, float *out, AH_SIntPtr *offsets, AH_SIntPtr n_samps, long n_chans, long chan, float mul);
void ibuffer_calc_samps_no_interp_float  (void *samps, float *out, AH_SIntPtr *offsets, AH_SIntPtr n_samps, long n_chans, long chan, float mul);

void ibuffer_samps_nointerp (void *samps, float *out, AH_SIntPtr *offsets, AH_SIntPtr n_samps, long n_chans, long chan, long format, float mul);

// Linear interopolation

void ibuffer_lin_interp_float (vFloat *out,  void **inbuffers, vFloat *fracts, AH_SIntPtr n_samps_over_4, float mul);
void ibuffer_lin_interp_int (vFloat *out, void **inbuffers, vFloat *fracts, AH_SIntPtr n_samps_over_4, float mul);

void ibuffer_calc_samps_lin_float (void *samps, vFloat *out, AH_SIntPtr *offsets, vFloat *fracts, void **temp, AH_SIntPtr n_samps, long n_chans, long chan, float mul);
void ibuffer_calc_samps_lin_16 (void *samps, vFloat *out, AH_SIntPtr *offsets, vFloat *fracts, void **temp, AH_SIntPtr n_samps, long n_chans, long chan, float mul);
void ibuffer_calc_samps_lin_24 (void *samps, vFloat *out, AH_SIntPtr *offsets, vFloat *fracts, void **temp, AH_SIntPtr n_samps, long n_chans, long chan, float mul);
void ibuffer_calc_samps_lin_32 (void *samps, vFloat *out, AH_SIntPtr *offsets, vFloat *fracts, void **temp, AH_SIntPtr n_samps, long n_chans, long chan, float mul);

void ibuffer_calc_samps_lin (void *samps, vFloat *out, AH_SIntPtr *offsets, vFloat *fracts, void **temp, AH_SIntPtr n_samps, long n_chans, long chan, long format, float mul);

// Cubic bspline interpolation

void ibuffer_cubic_bspline_float (vFloat *out, void **inbuffers, vFloat *fracts, AH_SIntPtr n_samps_over_4, float mul);
void ibuffer_cubic_bspline_int (vFloat *out,void **inbuffers, vFloat *fracts, AH_SIntPtr n_samps_over_4, float mul);

void ibuffer_calc_samps_cubic_bspline_float (void *samps, vFloat *out, AH_SIntPtr *offsets, vFloat *fracts, void **temp, AH_SIntPtr n_samps, long n_chans, long chan, float mul);
void ibuffer_calc_samps_cubic_bspline_16 (void *samps, vFloat *out, AH_SIntPtr *offsets, vFloat *fracts, void **temp, AH_SIntPtr n_samps, long n_chans, long chan, float mul);
void ibuffer_calc_samps_cubic_bspline_24 (void *samps, vFloat *out, AH_SIntPtr *offsets, vFloat *fracts, void **temp, AH_SIntPtr n_samps, long n_chans, long chan, float mul);
void ibuffer_calc_samps_cubic_bspline_32 (void *samps, vFloat *out, AH_SIntPtr *offsets, vFloat *fracts, void **temp, AH_SIntPtr n_samps, long n_chans, long chan, float mul);

void ibuffer_calc_samps_cubic_bspline (void *samps, vFloat *out, AH_SIntPtr *offsets, vFloat *fracts, void **temp, AH_SIntPtr n_samps, long n_chans, long chan, long format, float mul);

// Cubic hermite interpolation

void ibuffer_cubic_hermite_float (vFloat *out, void **inbuffers, vFloat *fracts, AH_SIntPtr n_samps_over_4, float mul);
void ibuffer_cubic_hermite_int (vFloat *out,void **inbuffers, vFloat *fracts, AH_SIntPtr n_samps_over_4, float mul);

void ibuffer_calc_samps_cubic_hermite_float (void *samps, vFloat *out, AH_SIntPtr *offsets, vFloat *fracts, void **temp, AH_SIntPtr n_samps, long n_chans, long chan, float mul);
void ibuffer_calc_samps_cubic_hermite_16 (void *samps, vFloat *out, AH_SIntPtr *offsets, vFloat *fracts, void **temp, AH_SIntPtr n_samps, long n_chans, long chan, float mul);
void ibuffer_calc_samps_cubic_hermite_24 (void *samps, vFloat *out, AH_SIntPtr *offsets, vFloat *fracts, void **temp, AH_SIntPtr n_samps, long n_chans, long chan, float mul);
void ibuffer_calc_samps_cubic_hermite_32 (void *samps, vFloat *out, AH_SIntPtr *offsets, vFloat *fracts, void **temp, AH_SIntPtr n_samps, long n_chans, long chan, float mul);

void ibuffer_calc_samps_cubic_hermite (void *samps, vFloat *out, AH_SIntPtr *offsets, vFloat *fracts, void **temp, AH_SIntPtr n_samps, long n_chans, long chan, long format, float mul);

// Cubic lagrange interpolation

void ibuffer_cubic_lagrange_float (vFloat *out, void **inbuffers, vFloat *fracts, AH_SIntPtr n_samps_over_4, float mul);
void ibuffer_cubic_lagrange_int (vFloat *out,void **inbuffers, vFloat *fracts, AH_SIntPtr n_samps_over_4, float mul);

void ibuffer_calc_samps_cubic_lagrange_float (void *samps, vFloat *out, AH_SIntPtr *offsets, vFloat *fracts, void **temp, AH_SIntPtr n_samps, long n_chans, long chan, float mul);
void ibuffer_calc_samps_cubic_lagrange_16 (void *samps, vFloat *out, AH_SIntPtr *offsets, vFloat *fracts, void **temp, AH_SIntPtr n_samps, long n_chans, long chan, float mul);
void ibuffer_calc_samps_cubic_lagrange_24 (void *samps, vFloat *out, AH_SIntPtr *offsets, vFloat *fracts, void **temp, AH_SIntPtr n_samps, long n_chans, long chan, float mul);
void ibuffer_calc_samps_cubic_lagrange_32 (void *samps, vFloat *out, AH_SIntPtr *offsets, vFloat *fracts, void **temp, AH_SIntPtr n_samps, long n_chans, long chan, float mul);

void ibuffer_calc_samps_cubic_lagrange (void *samps, vFloat *out, AH_SIntPtr *offsets, vFloat *fracts, void **temp, AH_SIntPtr n_samps, long n_chans, long chan, long format, float mul);


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////// Get ibuffer and related info //////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


static __inline void *ibuffer_get_ptr (t_symbol *s) FORCE_INLINE_DEFINITION
{
	t_object *b;
	
	if (!s) 
		return 0;
	
	b = s->s_thing;
	
	if (b && (ob_sym(b) == ps_ibuffer || ob_sym(b) == ps_buffer))
		return b;
	else 
		return 0;
}


static __inline long ibuffer_info (void *thebuffer, void **samples, AH_SIntPtr *length, long *channels, long *format) FORCE_INLINE_DEFINITION
{
	if (!thebuffer) 
		return 0;
	
	if (ob_sym(thebuffer) == ps_buffer)
	{
		t_buffer *buffer = thebuffer;
		if (buffer->b_valid)
		{
			*samples = (void *) buffer->b_samples;
			*length = buffer->b_frames;
			*channels = buffer->b_nchans;
			*format = PCM_FLOAT;
			return 1;
		}
	}
	else
	{
		t_ibuffer *buffer = thebuffer;
		if (buffer->valid)
		{
			*samples = buffer->samples;
			*length = buffer->frames;
			*channels = buffer->channels;
			*format = buffer->format;
			return 1;
		}
	}
	return 0;
}


static __inline double ibuffer_sample_rate (void *thebuffer) FORCE_INLINE_DEFINITION
{
	if (ob_sym(thebuffer) == ps_buffer)
		return (double) ((t_buffer *)thebuffer)->b_sr;
	else
		return (double) ((t_ibuffer *)thebuffer)->sr;		
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////// Inc /dec inuse counts /////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


static __inline void ibuffer_increment_inuse (void *thebuffer) FORCE_INLINE_DEFINITION
{
	if (ob_sym(thebuffer) == ps_buffer)
		ATOMIC_INCREMENT (&((t_buffer *)thebuffer)->b_inuse);
	else
		ATOMIC_INCREMENT (&((t_ibuffer *)thebuffer)->inuse);		
}


static __inline void ibuffer_decrement_inuse (void *thebuffer) FORCE_INLINE_DEFINITION
{
	if (ob_sym(thebuffer) == ps_buffer)
		ATOMIC_DECREMENT (&((t_buffer *)thebuffer)->b_inuse);
	else
		ATOMIC_DECREMENT (&((t_ibuffer *)thebuffer)->inuse);		
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////// Get individual samples /////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


static __inline float ibuffer_get_samp (void *samps, AH_SIntPtr offset, long n_chans, long chan, long format) FORCE_INLINE_DEFINITION
{	
	AH_UInt32 sampleint;
	
	switch (format)
	{
		case PCM_INT_16:
			sampleint = ((* (AH_UInt16 *) (((AH_UInt16 *) samps) + chan + (offset * n_chans) ) ) << 16) & MASK_16_BIT;
			return (float) *((AH_SInt32 *) &sampleint) * (float) TWO_POW_31_RECIP;			
			
		case PCM_INT_24:
			sampleint = * ( (AH_UInt32 *) ( ((AH_UInt8 *) samps) + (3 * (chan + (offset * n_chans))) - 1 )) & MASK_24_BIT;
			return (float) *((AH_SInt32 *) &sampleint) * (float) TWO_POW_31_RECIP;		
			
		case PCM_INT_32:
			return (float) ( *( ((AH_SInt32 *) samps) + chan + (offset * n_chans) ) ) * (float) TWO_POW_31_RECIP;
			
		case PCM_FLOAT:
			return *( ((float *) samps) + chan + (offset * n_chans) );
	}
	
	return 0.f;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// Add a fixed offset to the samps pointer ////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


static __inline void *ibuffer_offset (void *samps, AH_SIntPtr offset, long n_chans, long format) FORCE_INLINE_DEFINITION
{
	AH_SInt8 *samps_temp = samps;
	
	switch (format)
	{		
		case PCM_INT_16:
			return samps_temp + (offset * 2 * n_chans);
			
		case PCM_INT_24:
			return samps_temp + (offset * 3 * n_chans);
			
		case PCM_INT_32:
			return samps_temp + (offset * 4 * n_chans);
			
		case PCM_FLOAT:
			return samps_temp + (offset * 4 * n_chans);
	}
	
	return 0;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// Preprocess offsets according to format and number of channels /////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


static __inline void ibuffer_preprocess_offsets (AH_SIntPtr *offsets, AH_SIntPtr n_samps, long n_chans, long format) FORCE_INLINE_DEFINITION
{
	AH_SIntPtr i;
	long mul = n_chans;
	
	if (format == PCM_INT_24) 
		mul *= 3;
	
	if (mul != 1)
	{
		for (i = 0; i < n_samps; i++)
			*offsets++ *= mul;
	}
}


#endif	/* _IBUFFER_ACCESS_ */




