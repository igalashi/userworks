#include <iostream>

#include <cctype>
#include "koluri.h"

using namespace kol;

// Ref: RFC 3986

URI::URI(const char* struri)
{
  m_uri = 0;
  m_scheme = 0;
  m_user = 0;
  m_password = 0;
  m_hostport = 0;
  m_host = 0;
  m_port = -1;
  m_path = 0;
  m_query = 0;
  m_fragment = 0;

  const char* s;
  s = struri;
  s = set_scheme( s );
  s = set_authority( s );
  s = set_path( s );
  s = set_query( s );
  s = set_fragment( s );
}

URI::~URI()
{
  delete [] m_fragment;
  delete [] m_query;
  delete [] m_path;
  delete [] m_host;
  delete [] m_hostport;
  delete [] m_password;
  delete [] m_user;
  delete [] m_scheme;
  delete [] m_uri;
}

URI::URI(const URI& src)
{
  // Copy constructor
  m_uri = copy_chars(src.m_uri);
  m_scheme = copy_chars(src.m_scheme);
  m_user = copy_chars(src.m_user);
  m_password = copy_chars(src.m_password);
  m_hostport = copy_chars(src.m_hostport);
  m_host = copy_chars(src.m_host);
  m_path = copy_chars(src.m_path);
  m_query = copy_chars(src.m_query);
  m_fragment = copy_chars(src.m_fragment);
}

URI&
URI::operator=(const URI& src)
{
  if(&src == this)
    return *this;

  delete [] m_uri;
  m_uri = copy_chars(src.m_uri);

  delete [] m_scheme;
  m_scheme = copy_chars(src.m_scheme);

  delete [] m_user;
  m_user = copy_chars(src.m_user);

  delete [] m_password;
  m_password = copy_chars(src.m_password);

  delete [] m_hostport;
  m_hostport = copy_chars(src.m_hostport);

  delete [] m_host;
  m_host = copy_chars(src.m_host);

  delete [] m_path;
  m_path = copy_chars(src.m_path);

  delete [] m_query;
  m_query = copy_chars(src.m_query);

  delete [] m_fragment;
  m_fragment = copy_chars(src.m_fragment);

  return *this;
}

char*
URI::copy_chars(const char* s)
{
  if(s == 0)
    return 0;
  int n = 0;
  while(s[n])
    ++n;
  char* t = new char [(n + 1)];
  if(t == 0)
    return t;
  int i;
  for(i = 0; i < n; i++)
    t[i] = s[i];
  t[n] = 0;
  return t;
}

const char*
URI::set_scheme(const char* s)
{
  int n = 0;
  if( s == 0 )
    return s;

  // 1st character must be ALPHA
  if(!std::isalpha(s[0]))
    return s;

  // ALPHA / DIGIT / "+" / "-" / "."
  int c;
  while((c = s[++n]))
  {
    if((!std::isalnum(c)) &&
       (c != '+') && (c != '-') && (c != '.'))
      break;
  }
  if(c != ':')
    return s;
  if((m_scheme = new char [(n + 1)]))
  {
    int i;
    for( i = 0; i < n; i++ )
      m_scheme[i] = s[i];
    m_scheme[n] = 0;
  }
  return (s + n + 1);
}

const char*
URI::set_authority(const char* s)
{
  // RFC saids:
  // The authority component is preceded by a double slash ("//") and is terminated by the
  // next slash ("/"), question mark ("?"), or number sign ("#") character, or by the end of URI.

  if((s[0] != '/') || (s[1] != '/'))
    return s;

  int n = 2;
  int c;
  int ncol1 = 0;
  int ncol2 = 0;
  int nat = 0;

  while((c = s[n]))
  {
    if((c == '/') || (c == '?') || (c == '#'))
      break;
    if(c == ':')
    {
      if( nat == 0 )
        ncol1 = n;
      else
        ncol2 = n;
    }
    if(c == '@')
      nat = n;
    ++n;
  }
  if(nat == 0)
  {
    if(ncol1 >= 2)
      ncol2 = ncol1;
    ncol1 = 0;
  }
  else
  {
    if((ncol1 == 0) || (ncol1 > nat))
      ncol1 = nat;
  }
  
  // Authority is stored in the range [2..(n - 1)].
  //   user is stored [2..(ncol1 - 1)]
  //   password is stored [(ncol1 + 1)..(nat - 1)]
  //   host is stored [(nat + 1)..(ncol2 - 1)]
  //   port is stored [(ncol2 + 1)..(n - 1)]
  //      //user:password@host:port/

  if(nat == 0)
    nat = 1;
  if(ncol2 == 0)
    ncol2 = n;

  int ns;

  // user
  ns = ncol1 - 1 - 1;
  if( ns > 0 )
  {
    if((m_user = new char [(ns + 1)]))
    {
      int i;
      for( i = 0; i < ns; i++ )
        m_user[i] = s[2 + i];
      m_user[ns] = 0;
    }
  }

  // password
  ns = nat - 1 - ncol1;
  if( ns > 0 )
  {
    if((m_password = new char [(ns + 1)]))
    {
      int i;
      for( i = 0; i < ns; i++ )
        m_password[i] = s[ncol1 + 1 + i];
      m_password[ns] = 0;
    }
  }

  // hostport
  ns = n - 1 - nat;
  if( ns > 0 )
  {
    if((m_hostport = new char [(ns + 1)]))
    {
      int i;
      for( i = 0; i < ns; i++ )
        m_hostport[i] = s[nat + 1 + i];
      m_hostport[ns] = 0;
    }
  }

  // host
  ns = ncol2 - 1 - nat;
  if( ns > 0 )
  {
    if((m_host = new char [(ns + 1)]))
    {
      int i;
      for( i = 0; i < ns; i++ )
        m_host[i] = s[nat + 1 + i];
      m_host[ns] = 0;
    }
  }    

  // port
  ns = n - 1 - ncol2;
  if( ns > 0 )
  {
    int port = 0;
    int c;
    int i;
    for( i = 0; i < ns; i++ )
    {
      c = s[ncol2 + 1 + i];
      if(!std::isdigit(c))
        break;
      port = port * 10 + c - '0';
      m_port = port;
    }
  }
     
  return (s + n);
}

const char*
URI::set_path(const char* s)
{
  // RFC saids:
  // The path component is terminated by the
  // first question mark ("?"), or number sign ("#") character, or by the end of URI.

  if(s == 0)
    return s;
  if(s[0] == 0)
    return s;

  int n = 0;
  int c;
  while((c = s[n]))
  {
    if((c == '?') || (c == '#'))
      break;
    ++n;
  }
  if( n > 0 )
  {
    if((m_path = new char [(n + 1)]))
    {
      int i;
      for( i = 0; i < n; i++ )
        m_path[i] = s[i];
      m_path[n] = 0;
    }
  }
  return (s + n);
}

const char*
URI::set_query(const char* s)
{
  if(s == 0)
    return s;
  if(s[0] != '?')
    return s;

  int n = 1;
  int c;
  while((c = s[n]))
  {
    if(c == '#')
      break;
    ++n;
  }
  if( n > 1 )
  {
    if((m_query = new char [n]))
    {
      int i;
      for( i = 0; i < (n - 1); i++ )
        m_query[i] = s[i + 1];
      m_query[n - 1] = 0;
    }
  }
  return (s + n);
}

const char*
URI::set_fragment(const char* s)
{
  if(s == 0)
    return s;
  if(s[0] != '#')
    return s;

  int n = 1;
  int c;
  while((c = s[n]))
    ++n;
  if( n > 1 )
  {
    if((m_fragment = new char [n]))
    {
      int i;
      for( i = 0; i < (n - 1); i++ )
        m_fragment[i] = s[i + 1];
      m_fragment[n - 1] = 0;
    }
  }
  return (s + n);
}
