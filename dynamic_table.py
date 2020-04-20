import re
from utility import *

class dynamic_table() :
    """
    Class to produce a pretty table which is added to during the run. Headings are
    adjusted to fit in the space allocated to data.
    """

    class field() :

        def __init__(self, label, fmt, fn=None):
            self.text = var_to_title(label)
            self.fmt = fmt
            self.fn = fn if fn else lambda w: getattr(w, label)

        def __call__(self, w) -> str:
            t = self.fmt % self.fn(w)
            return ' ' * (self.width-len(t)) + t

    def __init__(self, fields, underline=True, file=None, console=False):
        """
        :param headings: list (or tuple) of 2-tuples, each containing
                         the text of the heading, and the % format string
                         to use for the data. The heading will be broken
                         at spaces as needed to fit the width indicated by
                         the format string.
        :param underline: if True (default), the headings are underlined
        :param f: file-like object (supports .write() ) to receive the
                  output. stdout is used if not supplied.
        """
        self.fields, self.underline, self.console = fields, file, console
        if isinstance(file, (list, tuple)) :
            self.file = file
        elif file :
            self.file = (file,)
        else :
            self.file = tuple()
        self.formats = []
        self.headings = []
        self.widths = []
        max_lines = 1
        for f in self.fields :
            text = f.text
            fmt = f.fmt
            m = re.match(r'%(\d+)(.*)', fmt)
            w = int(m.group(1))
            if len(text) <= w :
                tt = [text]
            else :
                tt = text.split(' ');
                busy = True
                while busy :
                    busy = False
                    max_w = max([len(ttt) for ttt in tt])
                    for i in range(len(tt)-1) :
                        if len(tt[i])+len(tt[i+1])+1 <= max_w :
                            tt[i] += ' ' + tt[i+1]
                            del tt[i+1]
                            busy = True
                            break
            w = max(w, *[len(ttt) for ttt in tt])
            self.widths.append(w)
            self.formats.append('%%%d%s' % (w, m.group(2)))
            self.headings.append([ ttt for ttt in reversed(tt) ])
            f.width = w
        max_lines = max([ len(tt) for tt in self.headings ])
        for tt in self.headings :
            while len(tt) < max_lines :
                tt.append(' ')
        pad = [''] * max_lines
        for i in reversed(range(max_lines)) :
            hl = []
            for w,tt in zip(self.widths, self.headings) :
                h = tt[i]
                hi = ' ' * (w - len(h)) + h
                hl.append(hi)
            self.write(hl)
        if self.underline :
            self.write(['-'*w for w in self.widths])

    def add(self, *data) -> None:
        """
        Add a line to the table.
        :param data: list/tuple of data values, of the same length and
                     types as required by the heading
        """
        self.write([ f % (d,) for f,d in zip(self.formats, data) ])

    def add_line(self, w):
        self.write([ f(w) for f in self.fields ])

    def write(self, items=[]):
        for f in self.file :
            f.write(' '.join(items) + '\n')
        if self.console :
            print(' '.join(items))

