////////////////////////////////////////////////////////////////////////////////
// taskwarrior - a command line task list manager.
//
// Copyright 2010, Johannes Schlatow.
// All rights reserved.
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation; either version 2 of the License, or (at your option) any later
// version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the
//
//     Free Software Foundation, Inc.,
//     51 Franklin Street, Fifth Floor,
//     Boston, MA
//     02110-1301
//     USA
//
////////////////////////////////////////////////////////////////////////////////

#include "Context.h"
#include "Uri.h"

extern Context context;

////////////////////////////////////////////////////////////////////////////////
Uri::Uri ()
{
}

////////////////////////////////////////////////////////////////////////////////
Uri::Uri (const Uri& other)
{
  if (this != &other)
  {
    data = other.data;
    host = other.host;
    path = other.path;
    user = other.user;
    port = other.port;
    protocol = other.protocol;
  }
}

////////////////////////////////////////////////////////////////////////////////
Uri::Uri (const std::string& in, const std::string& configPrefix)
{
  data = in;
  if (configPrefix != "")
    expand(configPrefix);
}

////////////////////////////////////////////////////////////////////////////////
Uri::~Uri ()
{
}

////////////////////////////////////////////////////////////////////////////////
Uri& Uri::operator= (const Uri& other)
{
  if (this != &other)
  {
    this->data = other.data;
    this->host = other.host;
    this->path = other.path;
    this->user = other.user;
    this->port = other.port;
    this->protocol = other.protocol;
  }

  return *this;
}

////////////////////////////////////////////////////////////////////////////////
Uri::operator std::string () const
{
  return data;
}

////////////////////////////////////////////////////////////////////////////////
std::string Uri::name () const
{
  if (path.length ())
  {
    std::string::size_type slash = path.rfind ('/');
    if (slash != std::string::npos)
      return path.substr (slash + 1, std::string::npos);
  }

 return path;
}

////////////////////////////////////////////////////////////////////////////////
std::string Uri::parent () const
{
  if (path.length ())
  {
    std::string::size_type slash = path.rfind ('/');
    if (slash != std::string::npos)
      return path.substr (0, slash);
  }

  return "";
}

////////////////////////////////////////////////////////////////////////////////
std::string Uri::extension () const
{
  if (path.length ())
  {
    std::string::size_type dot = path.rfind ('.');
    if (dot != std::string::npos)
      return path.substr (dot + 1, std::string::npos);
  }

  return "";
}

////////////////////////////////////////////////////////////////////////////////
bool Uri::is_directory () const
{
  return (path == ".")
      || (path == "")
      || (path[path.length()-1] == '/');
}

////////////////////////////////////////////////////////////////////////////////
bool Uri::is_local () const
{
  return ( (data.find("://") == std::string::npos)
        && (data.find(":")   == std::string::npos) );
}

////////////////////////////////////////////////////////////////////////////////
bool Uri::append (const std::string& path)
{
  if (is_directory ())
  {
    this->path += path;
    return true;
  }
  else
    return false;
}

////////////////////////////////////////////////////////////////////////////////
bool Uri::expand (const std::string& configPrefix )
{
  std::string tmp;
  if (data.length ())
  {
    // try to replace argument with uri from config
    tmp = context.config.get (configPrefix + "." + data + ".uri");    
  }  
  else
  {
    // get default target from config
    tmp = context.config.get (configPrefix + ".default.uri");
  }
  
  if (tmp != "")
  {
    data = tmp;
    return true;
  }
    
  return false;
}

////////////////////////////////////////////////////////////////////////////////
void Uri::parse ()
{
  if (is_local ())
  {
    path = data;
    return;
  }  
  
  std::string::size_type pos;
	std::string uripart;
	std::string pathDelimiter = "/";

	user = "";
	port = "";

	// skip ^.*://
  if ((pos = data.find ("://")) != std::string::npos)
	{
		protocol = data.substr(0, pos);
		data = data.substr (pos+3);
		// standard syntax: protocol://[user@]host.xz[:port]/path/to/undo.data
		pathDelimiter = "/";
	}
	else
	{
		protocol = "ssh";
		// scp-like syntax: [user@]host.xz:path/to/undo.data
		pathDelimiter = ":";
	}

	// get host part
	if ((pos = data.find (pathDelimiter)) != std::string::npos)
	{
		host = data.substr (0, pos);
		path = data.substr (pos+1);
	}
	else
	{
		throw std::string ("Could not parse \""+data+"\"");
	}

	// parse host
	if ((pos = host.find ("@")) != std::string::npos)
	{
		user = host.substr (0, pos);
		host = host.substr (pos+1);
	}

	// remark: this find() will never be != npos for scp-like syntax
	// because we found pathDelimiter, which is ":", before
	if ((pos = host.find (":")) != std::string::npos)
	{
		port = host.substr (pos+1);
		host = host.substr (0,pos);
	}
}

////////////////////////////////////////////////////////////////////////////////