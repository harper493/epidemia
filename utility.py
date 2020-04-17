__all__ = ( 'sround', 'cmpfn', 'scale_list', 'camel_to_title', 'var_to_title', 'common_prefix', \
            'splice', 'column_layout', 'format_columns', \
            'get_console_width', 'show_time', 'get_user_ip', \
            'make_plural', 'make_singular', 'is_irregular_plural', 'make_indef_article' ,
            'contains_any', 'get_random_member',
            'parse_time_of_day', 'decay', 'make_safe_string', 'number', 'indent', 'make_dict',
            'assert_', 'count' )

import math
import string
import time
import re
import os
import sys
import subprocess
import socket 
from getpass import getpass
import warnings
import textwrap
import datetime
import readline
import numpy as np
import random

#
# sround - round a number to the specified number of digits with possible round up
#

def sround(value, digits=1, round_up=True) :
    remainder = value
    result = 0
    if value > 0 :
        while digits>0 :
            try :
                log = math.log10(remainder)
            except ValueError :
                break
            if log >= 0 :
                int_log = int(log)
            else :
                int_log = -int(1-log)
            one_digit = float(10**int_log)
            bound = int(remainder / one_digit) * one_digit
            result += bound
            remainder -= bound
            digits -= 1
        if round_up and result < value :
            result += one_digit
    return result
        
#
# cmpfn - given any two comparable objects, return a cmp-like comparison result for them
#

def cmpfn(a,b) :
    if a < b :
        return -1
    elif a > b :
        return 1
    else :
        return 0

#
# scale_list - scale a list (or anything that behaves like one)
#

def scale_list(l, scale) :
    for i in len(l) :
        l[i] *= scale

#
# camel_to_title - insert spaces before upper case letters
#
def camel_to_title(text) :
    result = ''
    prev_upper = True
    for c in text :
        if c.isupper() :
            if not prev_upper :
                prev_upper = True
                result += ' '
        else :
            prev_upper = False
        result += c
    return result
#
# var_to_title - convert an underscore separated name to title case
#
def var_to_title(text) :
    result = ''
    up = True
    for ch in text :
        if ch=='_' :
            ch = ' '
            up = True
        elif up :
            ch = ch.upper()
            up = False
        result += ch
    return result
#
# common_prefix - given a list of strings, return the longest
# prefix they all share
#
def common_prefix(strings) :
    return os.path.commonprefix(strings)

#
# splice - given two strings, return their concatenation but with any
# common overlap just once, e.g.
#
# splice('abcde', 'defgh') == 'abcdefgh'
#
def splice(head, tail) :
    result = head + tail
    for i in reversed(range(1, min(len(head), len(tail))+1)) :
        if head[-i:]==tail[:i] :
            result = head + tail[i:]
            break
    return result
#
# indent - indent all the lines in a multiline string
#
def indent(s, prefix: str='', size: int=0, make=None) -> str:
    p = prefix if prefix else ' ' * size
    return '\n'.join([ (make(l,i) if make else p) + l for i,l in enumerate(s.split('\n'))])
#
# column_layout - given a list of items, turn them into a multi-column
# listing
#
# Args:
#
# items:      a list of strings to be arranged
# columns:    number of columns to use
# width:      minimum column width, or None. Columns will be wider
#             if they need to be
# spacing:    minimum number of spaces between columns
# right_align: align text in each column to the right if True, else left
#
# Returns a string containig the multi-column result
#

def column_layout(items, columns=2, width=None, max_width=None, spacing=2, right_align=False, field_wrap=None) :
    if len(items)==0 :
        return ""
    items += [' '] * ((columns - (len(items) % columns)) % columns)
    columns = [ items[c::columns] for c in range(columns) ]
    return format_columns(columns, width=width, max_width=max_width,
                          field_wrap=field_wrap, spacing=spacing, underline=False, right_align=right_align)

#
# format_columns - convert a list of lists of strings into neatly formatted
# columns. The nested lists are the columns/
#
# Args:
#
# items:     the list of columns
# width:     column width, can be:
#            -- None - each column is set to the width of its widest element
#            -- an int - each column is set to this width
#            -- a list of widths for each column
# max_width: can be:
#            -- an int - maximum width of every column
#            -- a list of widths for each column
#            If an entry does not fit in the width, it is truncated and '...'
#            added to the end.
# spacing:   number of spaces between adjacent columns
# underline: if True, put an underline under each column heading
# right_align: if True, align columns to the right, otherwise to the left
#            Can also be a list of bools along the lines of width
# field_wrap If not None, wrap fields to multiple lines, and break/pad
#            the lines at the indicated delimiter string (which may be null)
# wrap:      line width to wrap last column to, or 0 to use the actual
#            terminal width, or None to not wrap
#
# Returns a string containing the beautifully formatted table
            
def format_columns(items, width=None, max_width=None, spacing=2, underline=False, \
                   right_align=True, field_wrap=None, wrap=0, squish_headers=False, align_top=True) :
#
# format_field - internal function used by format_columns. Given a min length, max length,
# right_alignment and text, pad or truncate the text as needed and return a list
# broken lines that fit within the max_width
#
    def _format_field(width, right_align, text, field_wrap) :
        truncation_marker = '...'
        if width and len(text) > width :
            if field_wrap is not None :
                label, sep, value = text.partition(field_wrap) if field_wrap else ('', '', text)
                label += sep
                value_length = max((width - len(label)), 10 if len(field_wrap) else 0)
                just = unicode.rjust if right_align else unicode.ljust
                values = [ just(unicode(v), value_length) for v in textwrap.wrap(value, value_length) ]
                if len(values):
                    result = [label + values[0]] + [v.rjust(width) for v in values[1:]]
                else :
                    result = [label]
            else :
                result = [text[:(width - len(truncation_marker))] + truncation_marker]
        else : 
            if right_align :
                result = [text.rjust(width)]
            else :
                result = [text.ljust(width)]
        return result
#
# Now the real function body...
#
    if wrap==0 :
        wrap = get_console_width()-3 # the -3 seems necessary...?
    if not isinstance(max_width, list) :
        max_width = [max_width] * len(items)
    if width is None :
        width = []
        for m, i in zip(max_width, items) :
            item_width = max([len(e) for e in i[1 if squish_headers else 0 :]])
            header_width = max([len(h) for h in i[0].split()]) if squish_headers else 0
            this_max_width = m or 999
            width += [min(this_max_width, max(item_width, header_width))]
    elif not isinstance(width, list) :
        width = [int(width)] * len(items)
    if not isinstance(right_align, list) :
        right_align = [right_align] * len(items)
    if wrap :
        max_width[-1] = max(10, wrap - sum(width[:-1]))
        width[-1] = min(max_width[-1], width[-1])
    if underline :
        items = [[i[0]] + ['-' * w] + i[1:] for i,w in zip(items, width)]
    spacer = ''.ljust(spacing)
    lines = []
    first_line = True
    for l in zip(*items) :
        #print '###', width, max_width, right_align, l
        broken_fields = [ _format_field(*j, field_wrap=field_wrap) for j in zip(width, right_align, l) ]
        max_broken = max([len(f) for f in broken_fields])
        vpad = [ [''.ljust(len(f[0]))] * (max_broken - len(f)) for f in broken_fields ]
        if align_top :
            broken2 = [ f + p for f,p in zip(broken_fields, vpad) ]
        else :
            broken2 = [ p + f for f,p in zip(broken_fields, vpad) ]
        lines += [ spacer.join(b).rstrip() for b in zip(*broken2) ]
    return '\n'.join(lines)

#
# get_console_width - get the width of the terminal window
#
def get_console_width() :
    try :
        return int(subprocess.Popen('stty size', shell=True, \
                                    stdout=subprocess.PIPE, stderr=subprocess.PIPE). \
                   stdout.read().split()[1])
    except :
        return 80

def show_time() :
    return time.strftime('%Y-%m-%d %H:%M:%S')
            
#
# get_user_ip - get the IP address for the user
#
# We get the source host by scanning the pinky output looking
# for our terminal. This may either be a straight IP address, or
# a DNS host name which we need to resolve to an address.
#
def get_user_ip() :
    result = ''
    try :
        device = subprocess.check_output('tty', shell=True)
    except :
        return ''
    m = re.match(r'^/dev/(.*)$', device)
    if m :
        try :
            pinky_output = subprocess.check_output('pinky').split('\n')
        except :
            return ''
        for p in pinky_output :
            ps = p.split()
            if m.group(1) in ps :
                try :
                    result = ps[-1]
                    break
                except :
                    pass
        if not re.match(r'^[0-9.]+$', result) :
            try :
                result = socket.gethostbyname(result)
            except :
                m = re.match(r'^(\d+-\d+-\d+-\d+)-.*$', result)
                if m :
                    result = m.group(1).replace('-', '.')
    return '127.0.0.1' if result=='localhost' else result
#
# contains_any - return true if the first collection contains anything in the
# second collection 
#
def contains_any(container, choices) :
    for ch in choices:
        if ch in container :
            return True
    return False

#
# make_plural - given a word, return its plural
#

_plural_transforms = [(r'(.*(?:s|z|ch|sh|x))$', r'\1es'),
                     (r'(.*)quy$', r'\1quies'),
                     (r'(.*[^aeiou])y$', r'\1ies'),
                     (r'(.*[aeiloru])f$', r'\1ves'),
                     (r'(.*i)fe$', r'\1ves'),
                     (r'(.*)man$', r'\1men'),
                     ]

_plural_irregulars = { 
    'ox':'oxen',
    'vax' : 'vaxen',
    'roof' : 'roofs',
    'turf' : 'turfs',
    'sheep' : 'sheep',
    'salmon' : 'salmon',
    'trout' : 'trout',
    'child' : 'children',
    'person' : 'people',
    'louse' : 'lice',
    'foot' : 'feet',
    'mouse' : 'mice',
    'goose' : 'geese',
    'tooth' : 'teeth',
    'aircraft' : 'aircraft',
    'hovercraft' : 'hovercraft',
    'potato' : 'potatoes',
    'tomato' : 'tomatoes',
    'phenomenon' : 'phenomena',
    'index' : 'indices',
    'matrix' : 'matrices',
    'vertex' : 'vertices',
    'axis' : 'axes',
    'crisis' : 'crises',
    'samurai' : 'samurai',
    'radius' : 'radii',
    'fungus' : 'fungi',
    'millennium' : 'millennia',
    }

_plural_cache = {}

def make_plural(singular, quantity=2) :
    if quantity==1 :
        return singular
    try :
        return _plural_cache[singular]
    except KeyError : pass
    if singular=='!@!' :
        return _plural_cache
    try :
        plural = _plural_irregulars[singular]
    except KeyError :
        for t in _plural_transforms :
            plural = re.sub(t[0], t[1], singular)
            if plural!=singular :
                break
        else :
            plural = singular + 's'
    _plural_cache[singular] = plural
    return plural
#
# make_singular - opposite of the above
#
# For the time being this does the bare minimum needed for the CLI
#

_singular_irregulars = { p:s for s,p in _plural_irregulars.items() }

_singular_transforms = [(r'(.*)ies$', r'\1y'),
                        #(r'(.*[aeiloru])ves$', r'\1f'),
                        #(r'(.*i)fe$', r'\1ves'),
                        (r'(.*)men$', r'\1man'),
                        (r'(.*)s$', r'\1'),
                       ] 

_singular_cache = {}

def is_irregular_plural(plural) :
    return make_singular(plural) + 's' != plural

def make_singular(plural) :
    try :
        return _singular_cache[plural]
    except KeyError : pass
    if plural=='!@!' :
        return _singular_cache
    try :
        singular = _singular_irregulars[plural]
    except KeyError :
        for t in _singular_transforms :
            singular = re.sub(t[0], t[1], plural)
            if singular!=plural :
                break
        else :
            singular = plural
    _singular_cache[plural] = singular
    return singular

#
# make_indef_article - return 'a' or 'an' as apropriate
# for the given string
#

_indef_vowel_irregulars = re.compile('|'.join([
    'ewe',
    'ewer',
    'u[^aeiou][aeiou]\w*',
])+'$')

_indef_vowel_irregulars_2 = re.compile('|'.join([
    'uni[cfltv][aeiou]\w*',
    'unio\w*',
])+'$')

_indef_vowel_regulars = re.compile('|'.join([
    'un\w*', 
]))

_indef_consonant_irregulars = re.compile('|'.join([
    'hour\w*',
    'honor\w*',
    'honour\w*',
    'honest\w*',
])+'$')

def make_indef_article(noun) :
    result = 'a'
    if noun[0] in 'aeiou' :
        result = 'an'
        m1 = _indef_vowel_irregulars.match(noun)
        if m1 :
            result = 'a'
            m2 = _indef_vowel_regulars.match(noun)
            if m2 :
                result = 'an'
                m3 = _indef_vowel_irregulars_2.match(noun)
                if m3 :
                    result = 'a'
    elif _indef_consonant_irregulars.match(noun) :
        result = 'an'
    return result
#
# parse_time_of_day - given a string in the form hh[:mm[:ss]] convert it to the
# corresponding time object, Raise ValueError if the string is not valid.
#

def parse_time_of_day(value) :
    result = None
    values = value.split(':')
    if len(values) <= 3 :
        values = (values + ['0', '0'])[:3]
        vints = []
        for vv, limit in zip(values, [24,60,60]) :
            if vv.isdigit() :
                try :
                    vint = int(vv)
                except ValueError :
                    break
                if vint<0 or vint>=limit :
                    break
                vints.append(vint)
            else :
                break
        else :
            result = datetime.time(*vints)
    if result is None :
        raise ValueError("'%s' is not a valid time of day" % (value,))
    return result
#
# get_random_member - get a random member of a collection, weighted
# according to the supplied function
#

def get_random_member(coll, key):
    return random.choices(coll, weights=[key(c) for c in coll])[0]

#
# decay - perform one step of an exponentialy smoothed moving average
# given the alpha value, which must be in the range 0-1. Higher gives
# preference to older values.
#

def decay(alpha, prev_value, new_value) :
    return prev_value * alpha + new_value * (1 - alpha)

#
# make_safe_string - given a unicode string, turn it into a str()
# with all non-ASCII characters replaced
#

def make_safe_string(ustr, subst='.') :
    return ''.join([ chr(ord(ch)) if ord(ch)<128 else subst for ch in ustr ])

#
# number() - convert a string to int or flat
#
def number(s: str) :
    if '.' in s or 'e' in s.lower() :
        return float(s)
    else :
        return int(s)
#
# make_dict - given an object and a list of attributes, create a dict where
# each attribute's value is identified by its name
#

def make_dict(obj, *attrs) :
    return { a:getattr(obj, a, None) for a in attrs }

#
# assert_ - check the condition and potentially hit a breakpoint if it occurs
#
def assert_(p) :
    if not p :
        xxx = 1

#
# count - return the number of entries in an iterable matching a predicate
#
def count(coll, pred) :
    return sum([ 1 for c in coll if pred(c)])