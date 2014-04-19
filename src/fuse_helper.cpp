
#include "khan.h"
#include <vector>
#include <string>
#include <string.h>
#include <algorithm>

#include "database.h"
#include "utils.h"

#include "fuse_helper.h"

extern vector<std::string> server_ids;


  void
map_path (std::string path, std::string fileid)
{
  /* cout << "in map_path" << endl;
   * cout << "path: " << path << " fileid: " << fileid << endl;
   */
  std::string token = "";
  std::string attr = "";
  std::stringstream ss2 (path.c_str ());
  while (getline (ss2, token, '/'))
  {
    /* cout << "got token " << token << endl; */
    if (strcmp (token.c_str (), "null") != 0)
    {
      if (attr.length () > 0)
      {
        /* cout << "mapping " << attr << " to " << token << endl; */
        database_setval (fileid, attr, token);
        if (attr == "location")
        {
          int pos =
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
  /* cout << "finished map_path" << endl << endl; */
}

  void
unmap_path (std::string path, std::string fileid)
{
  /* cout << "in unmap_path" << endl;
   *      * cout << "path: " << path << " fileid: " << fileid << endl;
   *           */
  std::string token = "";
  std::string attr = "";
  std::stringstream ss2 (path.c_str ());
  while (getline (ss2, token, '/'))
  {
    /* cout << "got token " << token << endl; */
    if (strcmp (token.c_str (), "null") != 0)
    {
      if (attr.length () > 0)
      {
        /* cout << "removing map " << attr << " to " << token << endl; */
        database_remove_val (fileid, attr, token);
        if (attr == "location")
        {
          int pos =
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
  /* cout << "finished unmap_path" << endl << endl; */
}

  void
dir_pop_stbuf (struct stat *stbuf, std::string contents)
{
  cout << "Dir pop STBUF called ! " << endl;
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
  /* cout << "starting split" << endl << flush; */
  std::vector < std::string > pieces = split (path, "/");
  /* cout << "starting process" << endl << flush; */
  for (int i = 0; i < pieces.size (); i++)
  {
    /* cout << "looking at " << pieces[i] << endl << flush; */
    if (pieces[i].at (0) == SELECTOR_C)
    {
      /* cout << "is a selector" << endl << flush; */
      std::vector < std::string > selectores = split (pieces[i], SELECTOR_S);
      pieces[i] = "";
      /* cout << selectores.size() << " selectors to be exact" << endl << flush; */
      for (int j = 0; j < selectores.size (); j++)
      {
        /* cout << "checking " << selectores[j] << endl << flush; */
        bool matched = false;
        std::string content = database_getvals ("attrs");
        /* cout << "content " << content << endl << flush; */
        std::vector < std::string > attr_vec = split (content, ":");
        /* cout << "vs " << attr_vec.size() << " attrs" << endl << flush; */
        /* for all attrs */
        for (int k = 0; k < attr_vec.size (); k++)
        {
          /* cout << "on " << attr_vec[k] << endl << flush; */
          std::string vals = database_getvals (attr_vec[k]);
          /* cout << "with " << vals << endl << flush; */
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
  /* cout << "selector path " << path << " resolved to " << ret << endl; */
  return ret;
}
