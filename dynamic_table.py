import re

class dynamic_table() :
    """
    Class to produce a pretty table which is added to during the run. Headings are
    adjusted to fit in the space allocated to data.
    """

    def __init__(self, headings, underline=True, f=None):
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
        self.underline, self.file = underline, f
        self.formats = []
        self.headings = []
        self.widths = []
        max_lines = 1
        for h in headings :
            text = h[0]
            f = h[1]
            m = re.match(r'%(\d+)(.*)', f)
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
            self._write(hl)
        if self.underline :
            self._write(['-'*w for w in self.widths])

    def add(self, *data) -> None:
        """
        Add a line to the table.
        :param data: list/tuple of data values, of the same length and
                     types as required by the heading
        """
        self._write([ f % (d,) for f,d in zip(self.formats, data) ])

    def _write(self, items):
        if self.file :
            self.file.write(' '.join(items) + '\n')
        else :
            print(' '.join(items))

