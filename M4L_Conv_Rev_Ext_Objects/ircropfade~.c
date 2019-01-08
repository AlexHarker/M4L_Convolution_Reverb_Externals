
#include <ext.h>
#include <ext_obex.h>
#include <z_dsp.h>

#include <HIRT_Buffer_Access.h>
#include <AH_Types.h>


void *this_class;


typedef struct _ircropfade
{
    t_pxobject x_obj;

    // Attributes

    t_atom_long read_chan;
    t_atom_long write_chan;
    t_atom_long resize;

    // Bang Outlet

    void *process_done;

} t_ircropfade;


void *ircropfade_new();
void ircropfade_free(t_ircropfade *x);
void ircropfade_assist(t_ircropfade *x, void *b, long m, long a, char *s);

void ircropfade_process(t_ircropfade *x, t_symbol *sym, long argc, t_atom *argv);
void ircropfade_process_internal(t_ircropfade *x, t_symbol *sym, short argc, t_atom *argv);


int main (void)
{
    this_class = class_new("ircropfade~",
                           (method) ircropfade_new,
                           (method)ircropfade_free,
                           sizeof(t_ircropfade),
                           0L,
                           0);

    class_addmethod(this_class, (method)ircropfade_process, "process", A_GIMME, 0L);

    class_addmethod(this_class, (method)ircropfade_assist, "assist", A_CANT, 0L);

    CLASS_STICKY_ATTR(this_class, "category", 0L, "Buffer");

    CLASS_ATTR_ATOM_LONG(this_class, "writechan", 0L, t_ircropfade, write_chan);
    CLASS_ATTR_FILTER_MIN(this_class, "writechan", 1);
    CLASS_ATTR_LABEL(this_class,"writechan", 0L, "Buffer Write Channel");

    CLASS_ATTR_ATOM_LONG(this_class, "resize", 0L, t_ircropfade, resize);
    CLASS_ATTR_STYLE_LABEL(this_class,"resize", 0L, "onoff","Buffer Resize");

    CLASS_ATTR_ATOM_LONG(this_class, "readchan", 0, t_ircropfade, read_chan);
    CLASS_ATTR_FILTER_MIN(this_class, "readchan", 1);
    CLASS_ATTR_LABEL(this_class,"readchan", 0, "Buffer Read Channel");

    CLASS_STICKY_ATTR_CLEAR(this_class, "category");

    class_register(CLASS_BOX, this_class);

    buffer_access_init();

    return 0;
}


void *ircropfade_new()
{
    t_ircropfade *x = (t_ircropfade *)object_alloc (this_class);

    x->process_done = bangout(x);

    x->read_chan = 1;
    x->write_chan = 1;
    x->resize = 1;

    return(x);
}


void ircropfade_free(t_ircropfade *x)
{
}


void ircropfade_assist(t_ircropfade *x, void *b, long m, long a, char *s)
{
    if (m == ASSIST_INLET)
        sprintf(s,"Instructions In");
    else
        sprintf(s,"Bang on Success");
}


// Arguments are - target buffer / source buffer / crop 1 (samps) / crop 2 (samps) / fade in start (samps) / fade in length (samps) / fade out end (samps) / fade out length (samps)
// N.B. the fades are allowed to fall outside of the crop points

void ircropfade_process(t_ircropfade *x, t_symbol *sym, long argc, t_atom *argv)
{
    long i;

    if (argc < 8)
    {
        object_error((t_object *)x, "not enough arguments to message process");
        return;
    }

    if (atom_gettype(argv) != A_SYM)
    {
        object_error((t_object *)x, "incorrect arguments to message process");
        return;
    }

    if (atom_gettype(argv + 1) != A_SYM)
    {
        object_error((t_object *)x, "incorrect arguments to message process");
        return;
    }

    for (i = 2; i < 8; i++)
        if (atom_gettype(argv + i) == A_SYM)
            object_error((t_object *)x, "expected number, but found symbol (using 0)");

    defer(x, (method) ircropfade_process_internal, 0, 8, argv);
}


double calculate_fade(double pos, double lo, double fade_recip)
{
    double fade = (pos - lo) * fade_recip;

    fade = fade < 0 ? 0 : fade;
    fade = fade > 1 ? 1 : fade;

    return fade;
}


void ircropfade_process_internal(t_ircropfade *x, t_symbol *sym, short argc, t_atom *argv)
{
    // Load arguments

    t_symbol *target = atom_getsym(argv++);
    t_symbol *source = atom_getsym(argv++);
    t_atom_long crop1 = atom_getlong(argv++);
    t_atom_long crop2 = atom_getlong(argv++);
    double fade_start = atom_getfloat(argv++);
    double in_length = atom_getfloat(argv++);
    double fade_end = atom_getfloat(argv++);
    double out_length = atom_getfloat(argv++);

    // Set fade variables

    double fade_in_lo = fade_start - 1;
    double fade_in_hi = in_length > 0 ? fade_start + in_length : fade_start;
    double fade_out_lo = fade_end;
    double fade_out_hi = out_length > 0 ? fade_end - out_length : fade_end - 1;
    double fade_in_recip = 1. / (fade_in_hi - fade_in_lo);
    double fade_out_recip = 1. / (fade_out_hi - fade_out_lo);

    float *temp1;
    double *temp2;

    t_buffer_write_error error;

    AH_SIntPtr full_length = buffer_length(source);
    AH_SIntPtr final_length;
    AH_SIntPtr i;

    t_atom_long read_chan = x->read_chan - 1;

    double sample_rate = 0;

    // Check source buffer

    if (buffer_check((t_object *) x, source, read_chan))
        return;
    sample_rate = buffer_sample_rate(source);

    crop1 = crop1 < 0 ? 0 : crop1;
    crop2 = crop2 < 0 ? 0 : crop2;
    crop1 = crop1 > full_length - 1 ? full_length - 1: crop1;
    crop2 = crop2 > full_length ? full_length : crop2;

    if (crop1 >= crop2)
        return;

    final_length = crop2 - crop1;

    // Allocate Memory

    temp1 = (float *) ALIGNED_MALLOC(full_length  * sizeof(float) + final_length * sizeof(double));
    temp2 = (double *) (temp1 + full_length);

    // Check momory allocation

    if (!temp1)
    {
        object_error((t_object *)x, "could not allocate temporary memory for processing");
        free(temp1);
        return;
    }

    // Read from buffer

    buffer_read(source, read_chan, (float *) temp1, full_length);

    // Copy with crops / fades to double precision version

    for (i = 0; i < final_length; i++)
    {
        double in_val = temp1[i + crop1];
        double fade_in = calculate_fade((double) (i + crop1), fade_in_lo, fade_in_recip);
        double fade_out = calculate_fade((double) (i + crop1), fade_out_lo, fade_out_recip);

        temp2[i] = in_val * fade_in * fade_out;
    }

    // Copy out to buffer

    error = buffer_write(target, temp2, final_length, x->write_chan - 1, x->resize, sample_rate, 1.);
    buffer_write_error((t_object *)x, target, error);

    // Free Resources

    ALIGNED_FREE(temp1);

    if (!error)
        outlet_bang(x->process_done);
}

