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
#include "config.h"

#include "dbconn.h"
#include "dbquery.h"
#include "templates.h"
#include "template.h"
#include "debug.h"
#include "samsconfig.h"

bool Templates::_loaded = false;
map<long, Template*> Templates::_list;
DBConn *Templates::_conn;                ///< Соединение с БД
bool Templates::_connection_owner;

bool Templates::load()
{
  if (_loaded)
    return true;

  return reload();
}

bool Templates::reload()
{
  DEBUG (DEBUG_TPL, "[" << __FUNCTION__ << "] ");

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

  DBQuery *query2 = _conn->newQuery ();
  if (!query2)
    {
      ERROR("Unable to create query.");
      delete query;
      return false;
    }

  DBQuery *query3 = _conn->newQuery ();
  if (!query3)
    {
      ERROR("Unable to create query.");
      delete query;
      delete query2;
      return false;
    }

  long s_trange_id;
  long s_tpl_id;
  char s_auth[5];
  long s_quote;
  long s_alldenied;
  char s_period[5];
  char s_clrdate[15];
  long s_redirect_id;

  if (!query->bindCol (1, DBQuery::T_LONG,  &s_tpl_id, 0))
    {
      delete query;
      delete query2;
      delete query3;
      return false;
    }
  if (!query->bindCol (2, DBQuery::T_CHAR,  s_auth, sizeof(s_auth)))
    {
      delete query;
      delete query2;
      delete query3;
      return false;
    }
  if (!query->bindCol (3, DBQuery::T_LONG,  &s_quote, 0))
    {
      delete query;
      delete query2;
      delete query3;
      return false;
    }
  if (!query->bindCol (4, DBQuery::T_LONG,  &s_alldenied, 0))
    {
      delete query;
      delete query2;
      delete query3;
      return false;
    }
  if (!query->bindCol (5, DBQuery::T_CHAR,  s_period, sizeof(s_period)))
    {
      delete query;
      delete query2;
      delete query3;
      return false;
    }
  if (!query->bindCol (6, DBQuery::T_CHAR,  s_clrdate, sizeof(s_clrdate)))
    {
      delete query;
      delete query2;
      delete query3;
      return false;
    }

  if (!query2->bindCol (1, DBQuery::T_LONG,  &s_trange_id, 0))
    {
      delete query;
      delete query2;
      delete query3;
      return false;
    }

  if (!query3->bindCol (1, DBQuery::T_LONG,  &s_redirect_id, 0))
    {
      delete query;
      delete query2;
      delete query3;
      return false;
    }

  if (!query->sendQueryDirect ("select s_shablon_id, s_auth, s_quote, s_alldenied, s_period, s_clrdate from shablon"))
    {
      delete query;
      delete query2;
      delete query3;
      return false;
    }

  basic_stringstream < char >sqlcmd;

  Template *tpl = NULL;
  _list.clear();
  string str_period;
  long period_days;
  while (query->fetch())
    {
      tpl = new Template(s_tpl_id);
      tpl->setAuth (s_auth);
      tpl->setQuote (s_quote);
      tpl->setAllDeny ( ((s_alldenied==0)?false:true) );
      str_period = s_period;
      if (str_period == "M")
        tpl->setPeriod (Template::PERIOD_MONTH, 0);
      else if (str_period == "W")
        tpl->setPeriod (Template::PERIOD_WEEK, 0);
      else
        {
          if (sscanf (str_period.c_str (), "%ld", &period_days) != 1)
            period_days = 10;
          tpl->setPeriod (Template::PERIOD_CUSTOM, period_days);
          tpl->setClearDate (s_clrdate);
        }
      _list[s_tpl_id] = tpl;

      sqlcmd.str("");
      sqlcmd << "select s_trange_id from sconfig_time where s_shablon_id=" << s_tpl_id;
      if (query2->sendQueryDirect (sqlcmd.str ()))
        {
          while (query2->fetch())
            {
              tpl->addTimeRange (s_trange_id);
            }
        }

      sqlcmd.str("");
      sqlcmd << "select a.s_redirect_id from sconfig a, shablon b where a.s_shablon_id=b.s_shablon_id and b.s_shablon_id=" << s_tpl_id;
      if (query3->sendQueryDirect (sqlcmd.str ()))
        {
          while (query3->fetch())
            {
              tpl->addUrlGroup (s_redirect_id);
            }
        }
    }
  delete query;
  delete query2;
  delete query3;
  _loaded = true;

  return true;
}

void Templates::useConnection (DBConn * conn)
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

void Templates::destroy()
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
  map < long, Template* >::iterator it;
  for (it = _list.begin (); it != _list.end (); it++)
    {
      delete (*it).second;
    }
  _list.clear ();
}

Template * Templates::getTemplate(long id)
{
  load();

  map < long, Template* >::iterator it = _list.find (id);
  if (it == _list.end ())
    {
      DEBUG (DEBUG_TPL, "[" << __FUNCTION__ << "] " << id << " not found");
      return NULL;
    }
  return (*it).second;
}

vector<long> Templates::getIds()
{
  load();

  vector<long> lst;
  map <long, Template*>::iterator it;
  for (it = _list.begin (); it != _list.end (); it++)
    {
      lst.push_back((*it).first);
    }
  return lst;
}
