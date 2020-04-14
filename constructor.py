#
# constructor - generic help for delegated constructors
#
# arg table is either a list of argument names, or a dict of args names and
# the associated handler function or None, or the default value
#
# args which are handled are removed from the dict.
#

class constructor(object):

    def __init__(self, arg_table, args={}, cmd_args=None, props=None, props_prefix=[]) :
        self.arg_table, self.args, self.cmd_args, self.props, self.props_prefix =  \
            arg_table, args, cmd_args, props, props_prefix

    def apply(self, obj) :

        def _add(name, fn, value) :
            if isinstance(fn, tuple) :
                fn = fn[0]
            if isinstance(fn, type) :
                value = (fn)(value)
            elif callable(fn) :
                value = (fn)(obj, value)
            setattr(obj, name, value)
            obj.args[name] = value

        obj.args = {}
        if not isinstance(self.arg_table, dict):
            arg_table = {a: None for a in self.arg_table}
        for k, a in self.arg_table.items():
            if k in self.args:
                _add(k, a, self.args[k])
                del self.args[k]
            elif self.cmd_args and k in self.cmd_args:
                _add(k, a, self.cmd_args[k])
            elif self.props and self.props.get(*(self.props_prefix + [k])):
                _add(k, a, self.props.get(*(self.props_prefix + [k])))
            elif isinstance(a, tuple):
                _add(k, a[0], a[1])
            elif callable(a):
                _add(k, a, None)
            else:
                _add(k, None, a)

    def add_default_arg(args, name, value) :
        if name not in args :
            args[name] = value

    def check_unused_args(kwargs) :
        if kwargs :
            raise NameError("unexpected args: " + ", ".join(kwargs.keys()))

    def copy_from(obj, other, kwargs={}) :
        for a in other.args.iterkeys() :
            if a not in kwargs :
                setattr(obj, getattr(other, a))

