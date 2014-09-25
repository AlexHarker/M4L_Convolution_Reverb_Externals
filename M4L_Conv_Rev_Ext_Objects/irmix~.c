
#include <ext.h>
#include <ext_obex.h>
#include <z_dsp.h>

#include <HIRT_Buffer_Access.h>
#include <AH_Types.h>


void *this_class;


typedef struct _irmix
{
    t_pxobject x_obj;
	
	// Attributes
	
	long read_chan;
	long write_chan;
	long resize;
		
	// Bang Outlet
	
	void *process_done;
	
} t_irmix;


void *irmix_new (t_symbol *s, short argc, t_atom *argv);
void irmix_free (t_irmix *x);
void irmix_assist (t_irmix *x, void *b, long m, long a, char *s);

void irmix_mix (t_irmix *x, t_symbol *sym, long argc, t_atom *argv);
void irmix_mix_internal (t_irmix *x, t_symbol *sym, short argc, t_atom *argv);


int main (void)
{
    this_class = class_new ("irmix~",
							(method) irmix_new, 
							(method)irmix_free, 
							sizeof(t_irmix), 
							0L,		
							A_GIMME,
							0);
		
	class_addmethod (this_class, (method)irmix_mix, "mix", A_GIMME, 0L);
		
	class_addmethod (this_class, (method)irmix_assist, "assist", A_CANT, 0L);
	
	CLASS_STICKY_ATTR(this_class, "category", 0L, "Buffer");
	
	CLASS_ATTR_LONG(this_class, "writechan", 0L, t_irmix, write_chan);
	CLASS_ATTR_FILTER_CLIP(this_class, "writechan", 1, 4);
	CLASS_ATTR_LABEL(this_class,"writechan", 0L, "Buffer Write Channel");
	
	CLASS_ATTR_LONG(this_class, "resize", 0L, t_irmix, resize);
	CLASS_ATTR_STYLE_LABEL(this_class,"resize", 0L, "onoff","Buffer Resize");
	
	CLASS_ATTR_LONG(this_class, "readchan", 0L, t_irmix, read_chan);	
	CLASS_ATTR_FILTER_CLIP(this_class, "readchan", 1, 4);
	CLASS_ATTR_LABEL(this_class,"readchan", 0L, "Buffer Read Channel");
	
	CLASS_STICKY_ATTR_CLEAR(this_class, "category");
	
	class_register(CLASS_BOX, this_class);
	
	buffer_access_init();
	
	return 0;
}


void *irmix_new (t_symbol *s, short argc, t_atom *argv)
{
    t_irmix *x = (t_irmix *)object_alloc (this_class);

	x->process_done = bangout(x);
							  	
	x->read_chan = 1;
	x->write_chan = 1;
	x->resize = 1;
	
	attr_args_process(x, argc, argv);

	return(x);
}


void irmix_free(t_irmix *x)
{	
}


void irmix_assist (t_irmix *x, void *b, long m, long a, char *s)
{
	if (m == ASSIST_INLET)
		sprintf(s,"Instructions In");
	else
		sprintf(s,"Bang on Success");
}


long irmix_check_number(t_atom *a)
{
	if (atom_gettype(a) == A_LONG || atom_gettype(a) == A_FLOAT)
		return 1;
	else
		return 0;
}


short irmix_params(t_object *x, t_symbol **in_bufs, t_atom_long *offsets, double *gains, AH_SIntPtr *lengths, short argc, t_atom *argv, short max_bufs, AH_SIntPtr *max_len_ret, double *sr_ret)
{
	AH_SIntPtr max_length = 0;
	AH_SIntPtr new_length;
	
	double sample_rate = 0;
	double new_sample_rate;
	double gain;
	
	t_symbol *buffer;
	t_atom_long offset;
	short i, j;
	
	if (argc > max_bufs)
		argc = max_bufs;
	
	if (!argc)
	{
		object_error(x, "no buffers specified");
		return 0;
	}
	
	for (i = 0, j = 0; i < argc; i++, j++)
	{
		gain = 1.;
		offset = 0;
		
		if (atom_gettype(argv + i) != A_SYM)
		{
			object_error(x, "name of buffer expected, but number given");
			return 0;
		}
		
		if (buffer_check(x, atom_getsym(argv + i)))
			return 0;
		
		new_length = buffer_length (atom_getsym(argv + i));
		new_sample_rate = buffer_sample_rate(atom_getsym(argv + i));
		
		if (new_length == 0)
		{
			object_error(x, "buffer %s has zero length ", atom_getsym(argv + i)->s_name);
			return 0;
		}
		
		buffer = atom_getsym(argv + i);
		
		if (i < argc && irmix_check_number(argv + i + 1)) 
			gain = atom_getfloat(argv + ++i);
		
		if (i < argc && irmix_check_number(argv + i + 1)) 
			offset = atom_getlong(argv + ++i);
		
		if (offset < 0)
			offset = 0;
			
		// Store name / length / gain / offset
		
		in_bufs[j] = buffer;
		lengths[j] = new_length;
		gains[j] = gain;
		offsets[j] = offset;
		
		new_length += offset;
		
		if (new_length > max_length)
			max_length = new_length;
		
		// Check sample rates
		
		if ((sample_rate != 0 && sample_rate != new_sample_rate) || new_sample_rate == 0.)
			object_warn(x, "sample rates do not match for all source buffers");
		else 
			sample_rate = new_sample_rate;
	}
	
	*max_len_ret = max_length;
	*sr_ret = sample_rate;
	
	return j;
}


// Arguments are - target buffer - followed by a set of iterms for each source buffer of - buffer / [linear gain] / [offset in samples]

void irmix_mix (t_irmix *x, t_symbol *sym, long argc, t_atom *argv)
{
	defer(x, (method) irmix_mix_internal, sym, (short) argc, argv);
}


void irmix_mix_internal (t_irmix *x, t_symbol *sym, short argc, t_atom *argv)
{
	float *temp;
	double *accum;
			
	double gains[1128];
	
	t_symbol *target;
	t_symbol *buffer_names[128];
	
	t_atom_long offsets[128];
	t_atom_long offset;
	
	AH_SIntPtr lengths[128];
	
	AH_SIntPtr num_buffers;
	AH_SIntPtr max_length;
	AH_SIntPtr i, j;
	
	double gain;
	double sample_rate = 0;
	
	t_buffer_write_error error;
	
	// Check there are some arguments
	
	if (!argc)
	{
		object_error((t_object *)x, "not enough arguments to message %s", sym->s_name);
		return;
	}	
	
	target = atom_getsym(argv++);
	argc--;
		
	// Check buffers, storing names and lengths +  calculate total / largest length
	
	num_buffers = irmix_params((t_object *)x, buffer_names, offsets, gains, lengths, argc, argv, 128, &max_length, &sample_rate);
	
	if (!num_buffers)
		return;
	
	// Allocate memory 
	
	temp = (float *) ALIGNED_MALLOC(max_length * (sizeof(float) + sizeof(double)));
	accum = (double *) (temp + max_length);
	
	// Check memory allocation
	
	if (!temp)
	{
		object_error((t_object *)x, "could not allocate temporary memory for processing");
		free(temp);
		return;
	}
	
	// Zero accumulation 
	
	for (j = 0; j < max_length; j++)
		accum[j] = 0.;
	
	// Average
	
	for (i = 0; i < num_buffers; i++)
	{		
		AH_SIntPtr read_length = buffer_read(buffer_names[i], x->read_chan - 1, (float *) temp, max_length);
		
		offset = offsets[i];
		gain = gains[i];
		
		// Accumulate
		
		for (j = 0; j < read_length; j++)
			accum[j + offset] += temp[j] * gain;
	}
	
	// Copy out to buffer
	
	error = buffer_write(target, accum, max_length, x->write_chan - 1, x->resize, sample_rate, 1.);
	buffer_write_error((t_object *)x, target, error);
	
	// Free Resources
	
	ALIGNED_FREE(temp);
	
	if (!error)
		outlet_bang(x->process_done);
}