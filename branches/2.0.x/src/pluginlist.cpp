/***************************************************************************
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "configmake.h"

#include <dlfcn.h>

#include "pluginlist.h"
#include "samsconfig.h"
#include "proxy.h"
#include "tools.h"
#include "dbconn.h"
#include "dbquery.h"
#include "debug.h"

bool PluginList::_loaded = false;
DBConn *PluginList::_conn;
bool PluginList::_connection_owner;
vector < Plugin * > PluginList::_plugins;

bool PluginList::reload ()
{
  vector<string> liblist;
  vector<string>::iterator it;

  destroy ();

  liblist = fileList (PKGLIBDIR, "*.so");

  for (it = liblist.begin (); it != liblist.end (); it++)
    {
      loadPlugin (*it);
    }

  _loaded = true;

  return true;
}

void PluginList::useConnection (DBConn * conn)
{
  if (_conn)
    {
      DEBUG (DEBUG6, "[" << __FUNCTION__ << "] Already using " << _conn);
      return;
    }
  if (conn)
    {
      DEBUG (DEBUG6, "[" << __FUNCTION__ << "] Using external connection " << conn);
      _conn = conn;
      _connection_owner = false;
    }
}

void PluginList::destroy ()
{
  if (_connection_owner && _conn)
    {
      DEBUG (DEBUG6, "[" << __FUNCTION__ << "] Destroy connection " << _conn);
      delete _conn;
      _conn = NULL;
    }
  else if (_conn)
    {
      DEBUG (DEBUG6, "[" << __FUNCTION__ << "] Not owner for connection " << _conn);
    }
  else
    {
      DEBUG (DEBUG6, "[" << __FUNCTION__ << "] Not connected");
    }
}

bool PluginList::updateInfo ()
{
  DEBUG (DEBUG6, "[" << __FUNCTION__ << "] ");

  if (!load ())
    return false;

  if (!_conn)
    {
      _conn = SamsConfig::newConnection ();
      if (!_conn)
        {
          ERROR ("Unable to create connection.");
          return false;
        }

      if (!_conn->connect ())
        {
          delete _conn;
          return false;
        }
      _connection_owner = true;
      DEBUG (DEBUG6, "[" << __FUNCTION__ << "] Using new connection " << _conn);
    }
    else
    {
      DEBUG (DEBUG6, "[" << __FUNCTION__ << "] Using old connection " << _conn);
    }

  DBQuery *query = _conn->newQuery ();
  if (!query)
    {
      ERROR("Unable to create query.");
      return false;
    }

  char s_version[10];
  basic_stringstream < char >sql_cmd;
  sql_cmd << "select s_version from websettings";

  if (!query->bindCol (1, DBQuery::T_CHAR, s_version, sizeof (s_version)))
    {
      delete query;
      return false;
    }
  if (!query->sendQueryDirect (sql_cmd.str()) )
    {
      delete query;
      return false;
    }
  if (!query->fetch ())
    {
      return false;
    }
  delete query;
  query = NULL;

  bool store_to_db = false;
  string str_db_ver = TrimSpaces(s_version);
  string str_pkg_ver = VERSION;

  if (str_db_ver.compare (0, 5, "1.9.9") == 0)
    {
      ERROR ("DB doesn't support sysinfo plugins.");
      store_to_db = false;
    }
  else if (str_db_ver.compare (2, 3, "9.9") == 0)
    {
      DEBUG (DEBUG3, "[" << __FUNCTION__ << "] " << "Internal database version.");
      store_to_db = true;
    }
  else if (str_db_ver.compare (0, 3, str_pkg_ver, 0, 3) != 0)
    {
      DEBUG (DEBUG3, "[" << __FUNCTION__ << "] " << "Database version accepted.");
      store_to_db = true;
    }
  else if (str_db_ver.compare (0, 5, str_pkg_ver, 0, 5) != 0)
    {
      ERROR ("Incompatible database version. Expected " << str_pkg_ver.substr (0, 5) << ", but got " << str_db_ver.substr (0, 5));
      store_to_db = false;
    }
  else
    {
      DEBUG (DEBUG3, "[" << __FUNCTION__ << "] " << "Database version ok.");
      store_to_db = true;
    }

  DBQuery *querySelect = NULL;
  DBQuery *queryUpdate = NULL;
  DBQuery *queryInsert = NULL;

  char sys_name[50];
  char sys_version[10];
  char sys_author[30];
  char sys_info[1024];
  char sys_date[20];
  long sys_status;

  int proxyid = Proxy::getId ();

  if (store_to_db)
    {
      querySelect = _conn->newQuery ();
      if (!querySelect)
        {
          ERROR("Unable to create query.");
          return false;
        }
      queryUpdate = _conn->newQuery ();
      if (!queryUpdate)
        {
          ERROR("Unable to create query.");
          delete querySelect;
          return false;
        }
      queryInsert = _conn->newQuery ();
      if (!queryInsert)
        {
          ERROR("Unable to create query.");
          delete querySelect;
          delete queryUpdate;
          return false;
        }

      if (!querySelect->bindCol (1, DBQuery::T_LONG, &sys_status, 0))
        {
          delete querySelect;
          delete queryUpdate;
          delete queryInsert;
          return false;
        }


      basic_stringstream < char > sysinfo_update_cmd;
      sysinfo_update_cmd << "update sysinfo set s_info=?, s_date=? where s_name=? and s_version=? and s_proxy_id=" << proxyid;
      if (!queryUpdate->prepareQuery (sysinfo_update_cmd.str ()))
        {
          delete queryUpdate;
          return false;
        }
      if (!queryUpdate->bindParam (1, DBQuery::T_CHAR, &sys_info, sizeof(sys_info)))
        {
          delete queryUpdate;
          return false;
        }
      if (!queryUpdate->bindParam (2, DBQuery::T_CHAR, &sys_date, sizeof(sys_date)))
        {
          delete queryUpdate;
          return false;
        }
      if (!queryUpdate->bindParam (3, DBQuery::T_CHAR, &sys_name, sizeof(sys_name)))
        {
          delete queryUpdate;
          return false;
        }
      if (!queryUpdate->bindParam (4, DBQuery::T_CHAR, &sys_version, sizeof(sys_version)))
        {
          delete queryUpdate;
          return false;
        }


      basic_stringstream < char > sysinfo_insert_cmd;
      sysinfo_insert_cmd << "insert into sysinfo (s_proxy_id, s_name, s_version, s_author, s_info, s_date, s_status)";
      sysinfo_insert_cmd << " values (" << proxyid << ", ?, ?, ?, ?, ?, 0)";
      if (!queryInsert->prepareQuery (sysinfo_insert_cmd.str ()))
        {
          return false;
        }
      if (!queryInsert->bindParam (1, DBQuery::T_CHAR, sys_name, sizeof (sys_name)))
        {
          return false;
        }
      if (!queryInsert->bindParam (2, DBQuery::T_CHAR, sys_version, sizeof (sys_version)))
        {
          return false;
        }
      if (!queryInsert->bindParam (3, DBQuery::T_CHAR, sys_author, sizeof (sys_author)))
        {
          return false;
        }
      if (!queryInsert->bindParam (4, DBQuery::T_CHAR, sys_info, sizeof (sys_info)))
        {
          return false;
        }
      if (!queryInsert->bindParam (5, DBQuery::T_CHAR, sys_date, sizeof (sys_date)))
        {
          return false;
        }



    }

  string plug_name;
  string plug_version;
  string plug_author;
  string plug_data;
  basic_stringstream < char > sysinfo_select_cmd;
  struct tm *date_time;
  time_t now;

  vector<Plugin*>::iterator it;
  for (it = _plugins.begin (); it != _plugins.end (); it++)
    {
      plug_name = (*((*it)->getName))();
      plug_version = (*((*it)->getVersion))();
      plug_data = (*((*it)->getInfo))();
      plug_author = (*((*it)->getAuthor))();

      now = time (NULL);
      date_time = localtime(&now);
      strftime (sys_date, sizeof (sys_date), "%Y-%m-%d %H:%M:%S", date_time);

      if (store_to_db)
        {
          sysinfo_select_cmd.str("");
          sysinfo_select_cmd << "select s_status from sysinfo where s_proxy_id=" << proxyid;
          sysinfo_select_cmd << " and s_name='" << plug_name << "'";
          sysinfo_select_cmd << " and s_version='" << plug_version << "'";
          if (!querySelect->sendQueryDirect (sysinfo_select_cmd.str ()))
            continue;

          strcpy(sys_name, plug_name.c_str());
          strcpy(sys_version, plug_version.c_str());
          strcpy(sys_author, plug_author.c_str());
          strcpy(sys_info, plug_data.c_str());


          if (!querySelect->fetch ()) // такой записи еще нет, нужно добавлять
            {
              queryInsert->sendQuery ();
            }
          else //такая запись существует, нужно обновлять
            {
              if (sys_status == 1) // обновляем если плагин активен
                {
                  queryUpdate->sendQuery ();
                }
            }
        }
      else
        {
          INFO (plug_name << " " << plug_version << " reports at " << sys_date << " the following: " << plug_data);
        }
    }

  return true;
}

bool PluginList::load ()
{
  if (_loaded)
    return true;

  return reload();
}

bool PluginList::loadPlugin (const string &path)
{
  Plugin *pl = NULL;

  char *error;

  pl = (Plugin*)malloc (sizeof(Plugin));
  pl->handle = dlopen (path.c_str (), RTLD_LAZY);

  if (!pl->handle)
    {
      ERROR (dlerror());
      free(pl);
      return false;
    }

  dlerror();    /* Clear any existing error */
  *(void **) (&(pl->getInfo)) = dlsym(pl->handle, "_Z11informationv");
  if ((error = dlerror()) != NULL)
    {
      ERROR (error);
      dlclose(pl->handle);
      free (pl);
      return false;
    }
  *(void **) (&(pl->getName)) = dlsym(pl->handle, "_Z4namev");
  if ((error = dlerror()) != NULL)
    {
      ERROR (error);
      dlclose(pl->handle);
      free (pl);
      return false;
    }
  *(void **) (&(pl->getVersion)) = dlsym(pl->handle, "_Z7versionv");
  if ((error = dlerror()) != NULL)
    {
      ERROR (error);
      dlclose(pl->handle);
      free (pl);
      return false;
    }
  *(void **) (&(pl->getAuthor)) = dlsym(pl->handle, "_Z6authorv");
  if ((error = dlerror()) != NULL)
    {
    }

  _plugins.push_back (pl);
  return true;
}
