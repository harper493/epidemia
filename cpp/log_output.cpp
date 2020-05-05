#include "log_output.h"
#include "formatted.h"

/************************************************************************
 * create - parse the column list, and open the output file
 ***********************************************************************/

void log_output::create(const string &fn, bool csv_, bool do_heading, const vector<pairss> &col_info)
{
    filename = fn;
    csv = csv_;
    for (const pairss &c : col_info) {
        columns.emplace_back(*this, c.first, c.second);
    }
    if (fn.empty() || fn=="stdout") {
        my_ostr = &std::cout;
        my_stream = false;
    } else {
        my_ostr = new std::ofstream(filename);
        my_stream = true;
    }
    if (do_heading) {
        bool first = true;
        for (const column &c : columns) {
            if (csv && !first) {
                (*my_ostr) << ",";
            } else {
                first = false;
            }
            string fmt = csv ? "%s" : formatted("%%%ds", c.width);
            (*my_ostr) << formatted(fmt, c.heading);
        }
        (*my_ostr) << "\n";
    }
}

/************************************************************************
 * close - close and delete the output stream
 ***********************************************************************/

void log_output::close()
{
    if (my_stream) {
        delete my_ostr;
    }
}

/************************************************************************
 * start_line - set up to start outputting a line
 ***********************************************************************/

void log_output::start_line()
{
    column_iter = columns.begin();
    first = true;
}

/************************************************************************
 * column constructor - parse the format string
 ***********************************************************************/

log_output::column::column(const log_output &logger, const string &fmt, const string &hdg)
    : format_str(fmt), heading(hdg)
{
    regex rx("(\\d*)(.*)(\\w)");
    smatch m;
    regex_match(format_str, m, rx);
    if (m[1].str().empty()) {
        width = 0;
    } else {
        width = lexical_cast<int>(m[1]);
    }
    if (logger.csv) {
        if (m[3]=="s") {
            format_.parse(string("\"%") + m[2] + m[3] + "\"");
        } else {
            format_.parse(string("%") + m[2] + m[3]);
        }
    } else {
        format_.parse(string("%" + format_str));
    }
}
