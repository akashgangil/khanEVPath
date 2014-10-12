
#include "khan.h"
#include <vector>
#include <string>
#include <string.h>
#include <algorithm>

#include "database.h"
#include "utils.h"

#include "fuse_helper.h"

extern std::vector < std::string > server_ids;


  void
map_path (std::string path, std::string fileid)
{
  std::string token = "";
  std::string attr = "";
  std::stringstream ss2 (path.c_str ());
  while (getline (ss2, token, '/'))
  {
    if (strcmp (token.c_str (), "null") != 0)
    {
      if (attr.length () > 0)
      {
        database_setval (fileid, attr, token);
        if (attr == "location")
        {
          unsigned pos =
            std::find (server_ids.begin (), server_ids.end (),
                token) - server_ids.begin ();
          if (pos < server_ids.size ())
          {
            database_setval (fileid, "server", servers.at (pos));
          }
        }
        attr = "";
      }
      else
      {
        attr = token;
      }
    }
  }
}

  void
unmap_path (std::string path, std::string fileid)
{
  std::string token = "";
  std::string attr = "";
  std::stringstream ss2 (path.c_str ());
  while (getline (ss2, token, '/'))
  {
    if (strcmp (token.c_str (), "null") != 0)
    {
      if (attr.length () > 0)
      {
        database_remove_val (fileid, attr, token);
        if (attr == "location")
        {
          unsigned pos =
            std::find (server_ids.begin(), server_ids.end(),
                token) - server_ids.begin ();
          if (pos < server_ids.size ())
          {
            database_remove_val (fileid, "server",
                servers.at (pos));
          }
        }
        attr = "";
      }
      else
      {
        attr = token;
      }
    }
  }
}

  void
dir_pop_stbuf (struct stat *stbuf, std::string contents)
{
  std::cout << "Dir pop STBUF called ! " << std::endl;
  time_t current_time;
  time (&current_time);
  stbuf->st_mode = S_IFDIR | 0755;
  stbuf->st_nlink = count_string (contents) + 2;
  stbuf->st_size = 4096;
  stbuf->st_mtime = current_time;
  stbuf->st_uid = getuid ();
  stbuf->st_gid = getgid ();
}

  void
file_pop_stbuf (struct stat *stbuf, std::string filename)
{
  time_t current_time;
  time (&current_time);
  stbuf->st_mode = S_IFREG | 0644;
  stbuf->st_nlink = 1;
  stbuf->st_size = get_file_size (filename);
  stbuf->st_mtime = current_time;
  stbuf->st_uid = getuid ();
  stbuf->st_gid = getgid ();
}

  std::string
resolve_selectors (std::string path)
{
  std::vector < std::string > pieces = split (path, "/");
  for (unsigned i = 0; i < pieces.size (); i++)
  {
    if (pieces[i].at (0) == SELECTOR_C)
    {
      std::vector < std::string > selectores = split (pieces[i], SELECTOR_S);
      pieces[i] = "";
      for (unsigned j = 0; j < selectores.size (); j++)
      {
        bool matched = false;
        std::string content = database_getvals ("attrs");
        std::vector < std::string > attr_vec = split (content, ":");
        /* for all attrs */
        for (unsigned k = 0; k < attr_vec.size (); k++)
        {
          std::string vals = database_getvals (attr_vec[k]);
          /* see if piece is in vals */
          if (content_has (vals, selectores[j]))
          {
            /* if so piece now equals attr/val */
            if (pieces[i].length () > 0)
            {
              pieces[i] += "/";
            }
            matched = true;
            pieces[i] += attr_vec[k] + "/" + selectores[j];
            break;
          }
        }
        if (!matched)
        {
          pieces[i] += "tags/" + selectores[j];
        }
      }
    }
  }
  std::string ret = join (pieces, "/");
  return ret;
}
