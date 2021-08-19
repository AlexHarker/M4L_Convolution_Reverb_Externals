
#include <ext.h>
#include <ext_obex.h>
#include <z_dsp.h>

#include <HIRT_Memory.hpp>
#include <HIRT_Buffer_Access.hpp>


t_class *this_class;


struct t_irmix
{
    t_pxobject x_obj;

    // Attributes

    t_atom_long read_chan;
    t_atom_long write_chan;
    t_atom_long resize;

    // Bang Outlet

    void *process_done;
};


void *irmix_new(t_symbol *s, short argc, t_atom *argv);
void irmix_free(t_irmix *x);
void irmix_assist(t_irmix *x, void *b, long m, long a, char *s);

void irmix_mix(t_irmix *x, t_symbol *sym, long argc, t_atom *argv);
void irmix_mix_internal(t_irmix *x, t_symbol *sym, short argc, t_atom *argv);


int C74_EXPORT main()
{
    this_class = class_new ("irmix~",
                            (method) irmix_new,
                            (method)irmix_free,
                            sizeof(t_irmix),
                            0L,
                            A_GIMME,
                            0);

    class_addmethod(this_class, (method)irmix_mix, "mix", A_GIMME, 0L);

    class_addmethod(this_class, (method)irmix_assist, "assist", A_CANT, 0L);

    CLASS_STICKY_ATTR(this_class, "category", 0L, "Buffer");

    CLASS_ATTR_ATOM_LONG(this_class, "writechan", 0L, t_irmix, write_chan);
    CLASS_ATTR_FILTER_MIN(this_class, "writechan", 1);
    CLASS_ATTR_LABEL(this_class,"writechan", 0L, "Buffer Write Channel");

    CLASS_ATTR_ATOM_LONG(this_class, "resize", 0L, t_irmix, resize);
    CLASS_ATTR_STYLE_LABEL(this_class,"resize", 0L, "onoff","Buffer Resize");

    CLASS_ATTR_ATOM_LONG(this_class, "readchan", 0L, t_irmix, read_chan);
    CLASS_ATTR_FILTER_MIN(this_class, "readchan", 1);
    CLASS_ATTR_LABEL(this_class,"readchan", 0L, "Buffer Read Channel");

    CLASS_STICKY_ATTR_CLEAR(this_class, "category");

    class_register(CLASS_BOX, this_class);

    return 0;
}


void *irmix_new(t_symbol *s, short argc, t_atom *argv)
{
    t_irmix *x = (t_irmix *)object_alloc (this_class);

    x->process_done = bangout(x);

    x->read_chan = 1;
    x->write_chan = 1;
    x->resize = 1;

    attr_args_process(x, argc, argv);

    return x;
}


void irmix_free(t_irmix *x)
{
}


void irmix_assist(t_irmix *x, void *b, long m, long a, char *s)
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


short irmix_params(t_irmix *x, t_symbol **in_bufs, t_atom_long *offsets, double *gains, t_ptr_int *lengths, short argc, t_atom *argv, short max_bufs, t_ptr_int *max_len_ret, double *sr_ret)
{
    t_ptr_int max_length = 0;
    double sample_rate = 0.0;

    short i, j;

    if (argc > max_bufs)
        argc = max_bufs;

    if (!argc)
    {
        object_error((t_object *)x, "no buffers specified");
        return 0;
    }

    for (i = 0, j = 0; i < argc; i++, j++)
    {
        double gain = 1.0;
        t_atom_long offset = 0;

        if (atom_gettype(argv + i) != A_SYM)
        {
            object_error((t_object *)x, "name of buffer expected, but number given");
            return 0;
        }

        if (buffer_check((t_object *)x, atom_getsym(argv + i), x->read_chan - 1))
            return 0;

        t_ptr_int new_length = buffer_length(atom_getsym(argv + i));
        double new_sample_rate = buffer_sample_rate(atom_getsym(argv + i));

        if (new_length == 0)
        {
            object_error((t_object *)x, "buffer %s has zero length ", atom_getsym(argv + i)->s_name);
            return 0;
        }

        t_symbol *buffer = atom_getsym(argv + i);

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
            object_warn((t_object *)x, "sample rates do not match for all source buffers");
        else
            sample_rate = new_sample_rate;
    }

    *max_len_ret = max_length;
    *sr_ret = sample_rate;

    return j;
}


// Arguments are - target buffer - followed by a set of iterms for each source buffer of - buffer / [linear gain] / [offset in samples]

void irmix_mix(t_irmix *x, t_symbol *sym, long argc, t_atom *argv)
{
    defer(x, (method) irmix_mix_internal, sym, (short) argc, argv);
}


void irmix_mix_internal(t_irmix *x, t_symbol *sym, short argc, t_atom *argv)
{
    double gains[1128];

    t_symbol *target;
    t_symbol *buffer_names[128];

    t_atom_long offsets[128];

    t_ptr_int lengths[128];
    t_ptr_int max_length;
    
    double sample_rate = 0;

    // Check there are some arguments

    if (!argc)
    {
        object_error((t_object *)x, "not enough arguments to message %s", sym->s_name);
        return;
    }

    target = atom_getsym(argv++);
    argc--;

    // Check buffers, storing names and lengths +  calculate total / largest length

    short num_buffers = irmix_params(x, buffer_names, offsets, gains, lengths, argc, argv, 128, &max_length, &sample_rate);

    if (!num_buffers)
        return;

    // Allocate memory

    temp_ptr<float> temp(max_length);
    temp_ptr<double> accum(max_length);

    // Check memory allocation

    if (!temp || !accum)
    {
        object_error((t_object *)x, "could not allocate temporary memory for processing");
        return;
    }

    // Zero accumulation

    for (t_ptr_int j = 0; j < max_length; j++)
        accum[j] = 0.0;

    // Mix

    for (short i = 0; i < num_buffers; i++)
    {
        t_ptr_int read_length = buffer_read(buffer_names[i], x->read_chan - 1, temp.get(), max_length);
        t_atom_long offset = offsets[i];
        double gain = gains[i];

        // Accumulate

        for (t_ptr_int j = 0; j < read_length; j++)
            accum[j + offset] += temp[j] * gain;
    }

    // Copy out to buffer

    auto error = buffer_write((t_object *)x, target, accum.get(), max_length, x->write_chan - 1, static_cast<long>(x->resize), sample_rate, 1.);

    if (!error)
        outlet_bang(x->process_done);
}

