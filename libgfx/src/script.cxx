#include <gfx/script.h>
#include <fstream>

#if !defined(HAVE_SSTREAM) && defined(HAVE_STRSTREAM)
#  include <strstream>
typedef istrstream istringstream;
#else
#  include <sstream>
#endif

#ifdef HAVE_GZSTREAM
#  include <gzstream.h>
#endif

namespace gfx
{

using namespace std;


////////////////////////////////////////////////////////////////////////
//
// CmdLine methods -- These implement all the basic line manipulations
//

string CmdLine::token_to_string(int i) const { return substr(tokens[i]); }

string CmdLine::rest_to_string(int i) const
{
    return line.substr(tokens[i].first);
}

double CmdLine::token_to_double(int i) const
    { string str = substr(tokens[i]); return atof(str.c_str()); }

float CmdLine::token_to_float(int i) const
    { string str = substr(tokens[i]); return atof(str.c_str()); }

int CmdLine::token_to_int(int i) const
    { string str = substr(tokens[i]); return atoi(str.c_str()); }

string CmdLine::argline() const
{
    if( argcount() == 0 )  return "";

    index_type start = tokens.front().first;
    index_type end = tokens.back().second;
    return substr(range_type(start, end));
}

int CmdLine::collect_as_strings(vector<string> &v, int offset) const
{
    for(int i=offset; i<tokens.size(); i++)
	v.push_back( substr(tokens[i]) );
    return tokens.size();
}

int CmdLine::collect_as_numbers(vector<double> &v, int offset) const
{
    for(int i=offset; i<tokens.size(); i++)
	v.push_back(token_to_double(i));
    return tokens.size();
}

int CmdLine::collect_as_numbers(vector<int> &v, int offset) const
{
    for(int i=offset; i<tokens.size(); i++)
	v.push_back(token_to_int(i));
    return tokens.size();
}

int CmdLine::collect_as_numbers(double *v, int size, int offset) const
{
    int i;
    for(i=0; (i+offset)<tokens.size() && i<size; i++)
	v[i] = token_to_double(i+offset);
    return i;
}

int CmdLine::collect_as_numbers(float *v, int size, int offset) const
{
    int i;
    for(i=0; (i+offset)<tokens.size() && i<size; i++)
	v[i] = token_to_float(i+offset);
    return i;
}

int CmdLine::collect_as_numbers(int *v, int size, int offset) const
{
    int i;
    for(i=0; (i+offset)<tokens.size() && i<size; i++)
	v[i] = token_to_int(i+offset);
    return i;
}

////////////////////////////////////////////////////////////////////////
//
// CmdEnv methods -- Minimal interface supported by all environments
//

void CmdEnv::register_command(const std::string& name, CmdObject *fn)
{
    script_commands[name] = fn;
}

void CmdEnv::register_command(const std::string& name, CmdHandler proc)
{
    register_command(name, new CmdFunction(proc));
}

CmdObject *CmdEnv::lookup_command(const std::string& name)
{
    CmdTable::const_iterator iter = script_commands.find(name);

    return iter!=script_commands.end() ? iter->second : NULL;
}

static int ignored(const CmdLine& line) { return SCRIPT_OK; }

void CmdEnv::ignore_command(const std::string& name)
{
    register_command(name, ignored);
}

void CmdEnv::register_vocabulary(const std::string& name, CmdEnv *env)
{
    register_method(name, env, &CmdEnv::script_eval);
}

CmdEnv::CmdEnv()
{
    register_method("include", this, &CmdEnv::script_include);
    register_method("ignore", this, &CmdEnv::script_ignore);
    register_method("end", this, &CmdEnv::script_end);
}

CmdEnv::~CmdEnv()
{
    // Free all the CmdObject's held by script_commands
    //
    for(CmdTable::iterator i = script_commands.begin();
	i != script_commands.end(); ++i)
    {
	CmdObject *obj = i->second;
	delete obj;
    }
}

int CmdEnv::script_include(const CmdLine& cmd)
{
    if( cmd.argcount() != 1 )  return SCRIPT_ERR_SYNTAX;

    string filename = cmd.token_to_string(0);
    return do_file(cmd.token_to_string(0));
}

int CmdEnv::script_ignore(const CmdLine& cmd)
{
    for(int i=0; i<cmd.argcount(); ++i)
    {
	string name = cmd.token_to_string(i);
	ignore_command(name);
    }

    return SCRIPT_OK;
}

int CmdEnv::script_end(const CmdLine& cmd)
{
    return SCRIPT_END;
}

int CmdEnv::script_eval(const CmdLine& cmd)
{
    return do_line(cmd.argline());
}

void CmdEnv::begin_scope(CmdEnv *sub) { scopes.push_back(sub); }

void CmdEnv::end_scope()
{
    if( scopes.size() > 0 )
    {
	CmdEnv *sub = scopes.back();
	scopes.pop_back();
	delete sub;
    }
}

////////////////////////////////////////////////////////////////////////
//
// Toplevel functions -- extract and execute scripting commands
//

int CmdEnv::do_line(const string &line)
{
    CmdEnv& env = *this;

    // Pass this line off to the sub-scope (if any)
    if( scopes.size() > 0 && scopes.back() )
    {
	int rc = scopes.back()->do_line(line);
	if( rc==SCRIPT_END )
	{
	    end_scope();
	    rc = SCRIPT_OK;
	}
	return rc;
    }

    const char *ws = " \t\n\r";
    string::size_type start, end;

    // First, process the initial (command) token
    start = line.find_first_not_of(ws);

    // Only continue processing if line is a non-empty, non-comment line
    if( start!=string::npos && line[start]!='#' )
    {
	end = line.find_first_of(ws, start);
	string op = line.substr(start, end-start);

	CmdObject *fn = env.lookup_command(op);
	if( !fn )  return SCRIPT_ERR_UNDEF;

	CmdLine argv(line);
	argv.op = CmdLine::range_type(start, end);

	while(1)
	{
	    start = line.find_first_not_of(ws, end);
	    if( start==string::npos )  break;
	    end = line.find_first_of(ws, start);

	    argv.tokens.push_back( CmdLine::range_type(start, end) );
	}

	return (*fn)(argv);
    }

    return SCRIPT_OK;
}

int CmdEnv::do_stream(istream &in)
{
    string line;

    while( !in.eof() )
    {
	getline(in, line);
	if( in.fail() )  break;

	int rc = do_line(line);
	if( rc != SCRIPT_OK )
	{
	    cerr << "Script Error: " << line << endl;
	    return rc;
	}
    }

    return SCRIPT_OK;
}

int CmdEnv::do_file(const std::string& filename)
{
#ifdef HAVE_GZSTREAM
    if( !filename.compare(filename.size()-3, 3, ".gz") ||
        !filename.compare(filename.size()-2, 2, ".z")  ||
        !filename.compare(filename.size()-2, 2, ".Z")  )
    {
	igzstream in(filename.c_str());
	if( in.good() )  return do_stream(in);
	else             return SCRIPT_ERR_NOFILE;
    }
    else
    {
	ifstream in(filename.c_str());
	if( in.good() )  return do_stream(in);
	else             return SCRIPT_ERR_NOFILE;
    }
#else
    ifstream in(filename.c_str());
    if( in.good() )  return do_stream(in);
    else             return SCRIPT_ERR_NOFILE;
#endif
}

int CmdEnv::do_string(const std::string& str)
{
    istringstream in(str.c_str());
    if( in.good() )  return CmdEnv::do_stream(in);
    else             return SCRIPT_ERR_NOFILE;
}

} // namespace gfx
