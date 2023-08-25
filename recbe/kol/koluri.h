#ifndef KOLURI_H_INCLUDED
#define KOLURI_H_INCLUDED

namespace kol
{
  // Uniform Resource Identifier
  // Ref: RFC 3986

  class URI
  {
  public:
    URI(const char* struri);
    virtual ~URI();
    URI(const URI& src);
    URI& operator=(const URI& src);
    const char* scheme() const { return m_scheme; }
    const char* user() const { return m_user; }
    const char* password() const { return m_password; }
    const char* hostport() const { return m_hostport; }
    const char* host() const { return m_host; }
    int port() const { return m_port; }
    const char* path() const { return m_path; }
    const char* query() const { return m_query; }
    const char* fragment() const { return m_fragment; }

  private:
    const char* set_scheme(const char* s);
    const char* set_authority(const char* s);
    const char* set_path(const char* s);
    const char* set_query(const char* s);
    const char* set_fragment(const char* s);
    char* copy_chars(const char* s);

  private:
    char* m_uri;
    char* m_scheme;
    char* m_user;
    char* m_password;
    char* m_hostport;
    char* m_host;
    int m_port;
    char* m_path;
    char* m_query;
    char* m_fragment;
  };
}    

#endif  /* KOLURI_H_INCLUDED */
