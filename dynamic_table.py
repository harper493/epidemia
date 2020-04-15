import re

class dynamic_table() :

    def __init__(self, headings, underline=True, f=None):
        self.underline, self.file = underline, f
        self.formats = []
        self.headings = []
        for h in headings :
            text = h[0]
            f = h[1]
            m = re.match(r'%(\d+)(.*)', f)
            w = max(len(text), int(m.group(1)))
            self.formats.append('%%%d%s' % (w, m.group(2)))
            self.headings.append(' '*(w - len(text)) + text)
        self._write(self.headings)
        if self.underline :
            self._write(['-'*len(h) for h in self.headings])

    def add(self, *data):
        self._write([ f % (d,) for f,d in zip(self.formats, data) ])

    def _write(self, items):
        if self.file :
            self.file.write(' '.join(items) + '\n')
        else :
            print(' '.join(items))

