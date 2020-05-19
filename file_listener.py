from threading import Lock, Thread
from datetime import datetime
import time


class file_listener():

    def __init__(self, filename):
        self.filename = filename
        self.ended = False
        self.lines = []
        self.lock = Lock()
        self.thread = Thread(target=self.run, name='file_listener-' + filename)
        self.thread.start()

    def run(self):

        def _append_line(line):
            with self.lock:
                if line.startswith("#!End"):
                    self.lines.append(None)
                    self.ended = True
                else:
                    self.lines.append(line)

        while True:  # loop until we can open the file
            try:
                self.file = open(self.filename, 'r')
                break
            except IOError:
                time.sleep(0.1)
        while not self.ended:  # read all the lines already there
            line = self.file.readline()
            if line:
                _append_line(line)
            else:
                break
        while not self.ended:  # now read lines as they become available
            where = self.file.tell()
            line = self.file.readline()
            if line:
                _append_line(line)
            else:
                time.sleep(0.1)
                self.file.seek(where)

    def __iter__(self):
        lineno = 0
        while True:
            with self.lock:
                if lineno < len(self.lines):
                    line = self.lines[lineno]
                    if line:
                        yield line
                    else:
                        self.thread.join()
                        return
                    lineno += 1
                    continue
            time.sleep(0.1)

    def join(self):
        self.thread.join()

