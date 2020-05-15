from threading import Lock, Thread
import csv

class csv_listener():

    def __init__(self, file, fn, run=True):
        self.file = file
        self.fn = fn
        self.thread = None
        if run:
           self.run()

    def run(self):
        self.thread = Thread(target=self.listen)
        self.thread.start()

    def listen(self):
        for row in csv.DictReader(self.file):
            self.fn(row)
        self.fn(None)

    def join(self):
        self.thread.join()