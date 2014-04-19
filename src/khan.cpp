//mkdir stats prints stats to stats file and console:
//std::string stats_file="./stats.txt";
//std::string rename_times_file_name="./rename_times.txt";
//std::string start_times_file_name = "./start_times.txt";
//std::vector<std::string> servers;
//std::string this_server;
//std::string this_server_id;
//
//static std::string primary_attribute = "";
//
////PyObject* cloud_interface;
//ofstream rename_times;
//ofstream start_times;  

#include <string>
#include <stdio.h>
#include <vector>
#include <iostream>
#include <errno.h>
#include <glob.h>
#include <set>
#include <sys/types.h>
#include <dirent.h>


#include "khan.h"
#include "data_analytics.h"
#include "fileprocessor.h"
#include "database.h"
#include "utils.h"
std::vector<std::string> servers;
std::vector<std::string> server_ids;

std::string primary_attribute = "";

std::string mountpoint;

extern struct fuse_operations khan_ops;
extern char msg[4096];
  void *
initializing_khan (void *mnt_dir)
{
  log_msg ("In initialize\n");
  /* unmounting((char *)mnt_dir); */
  /* Opening root directory and creating if not present */
  sprintf (msg, "khan_root[0] is %s\n", servers.at (0).c_str ());
  log_msg (msg);
  /* std::cout<<"khan_root[0] is "<<servers.at(0)<<endl; */
  if (NULL == opendir (servers.at (0).c_str ()))
  {
    sprintf (msg, "Error msg on opening directory : %s\n",
        strerror (errno));
    log_msg (msg);
    log_msg ("Root directory might not exist..Creating\n");
    std::string command = "mkdir " + servers.at (0);
    if (system (command.c_str ()) < 0)
    {
      log_msg ("Unable to create storage directory...Aborting\n");
      exit (1);
    }
  }
  else
  {
    log_msg ("directory opened successfully\n");
  }

  init_database ();

  /* check if we've loaded metadata before */
  std::string output = database_getval ("setup", "value");
  if (output.compare ("true") == 0)
  {
    log_msg ("Database was previously initialized.");
    tot_time +=
      (stop.tv_sec - start.tv_sec) + (stop.tv_nsec -
          start.tv_nsec) / BILLION;
    return 0;			/* setup has happened before */
  }

  /* if we have not setup, do so now */
  log_msg ("it hasnt happened, setvalue then setup\n");
  database_setval ("setup", "value", "true");

  /* load metadata associatons */
  for (int i = 0; i < servers.size (); i++)
  {
    /* log_msg("servers: " + servers.at(i) + "\n"); */
    process_transducers (servers.at (i));
  }

  /* load metadata for each file on each server */
  std::string types = database_getval ("allfiles", "types");
  sprintf (msg, "=== types to look for = %s\n", types.c_str ());
  log_msg (msg);

  /* log_msg("Server Size" + servers.size() + "\n"); */
  for (int i = 0; i < servers.size (); i++)
  {
    /*    if(servers.at(i) == "cloud") {
     *      std::cout << " Cloud \n";
     *      PyObject* myFunction = PyObject_GetAttrString(cloud_interface,(char*)"get_all_titles");
     *      PyObject* myResult = PyObject_CallObject(myFunction, NULL);
     *      if(myResult==NULL) {
     *         PyErr_PrintEx(0);
     *         continue;
     *      }
     *      int n = PyList_Size(myResult);
     *      std::cout << "SIZE = " << n << endl << flush; 
     */
    /*for (int j = 0; j < n; j++)
    {
      PyObject *title = PyList_GetItem (myResult, j);
      char *temp = PyString_AsString (title);
      if (temp == NULL)
      {
        PyErr_PrintEx (0);
        continue;
      }
      std::string filename = temp;
      /* std::cout << "Checking " << filename << " ... " << endl << flush; */
      /*if (database_getval ("name", filename) == "null")
      {
        std::string fileid = database_setval ("null", "name", filename);
        std::string ext = strrchr (filename.c_str (), '.') + 1;
        database_setval (fileid, "ext", ext);
        database_setval (fileid, "server", servers.at (i));
        database_setval (fileid, "location", server_ids.at (i));
        std::string attrs = database_getval (ext, "attrs");
        std::string token = "";
        std::stringstream ss2 (attrs.c_str ());
        PyObject *myFunction =
          PyObject_GetAttrString (cloud_interface,
              (char *) "get_metadata");
        while (getline (ss2, token, ':'))
        {
          if (strcmp (token.c_str (), "null") != 0)
          {

            PyObject *arglist = PyTuple_New (2);
            PyTuple_SetItem (arglist, 0,
                PyString_FromString (filename.
                  c_str ()));
            PyTuple_SetItem (arglist, 1,
                PyString_FromString (token.c_str ()));
            PyObject *myResult =
              PyObject_CallObject (myFunction, arglist);

            if (myResult == NULL)
            {
              PyErr_PrintEx (0);
              continue;
            }
            char *msg = PyString_AsString (myResult);
            if (!msg)
            {
              PyErr_PrintEx (0);
              continue;
            }
            std::string val = msg;
            Py_DECREF (arglist);
            Py_DECREF (myResult);

            if (val != "na")
            {
              database_setval (fileid, token, val);
            }
          }
        }
      }
    }
  }
  else
  {*/
    log_msg ("Not Cloud \n");

    /*      std::string command = "find -type d | awk -F'/' '{print NF-1}' | sort -n | tail -1"; */
    /* std::string command = "find /net/hp100/ihpcae -type d | awk -F'/' '{print NF-1}' | sort -n | tail -1"; */

    glob_t files;
    std::string pattern = servers.at (0) + "/*";
    static int experiment_id = 0;
    set < std::string > experiments;

    for (int count = 18; count > 0; count--)
    {
      sprintf (msg, "Globbing with pattern: %s .im7\n", pattern.c_str ());
      /* log_msg("Globbing with pattern " + pattern + ".im7\n"); */
      log_msg (msg);
      glob ((pattern + ".im7").c_str (), 0, NULL, &files);

      sprintf (msg, "Glob Buffer: %d\n", files.gl_pathc);
      log_msg (msg);

      /*log_msg("Glob buffer" + files.gl_pathc + "\n");
       *                      if(files.gl_pathc != 0 ) {
       *                                               experiment_id++;
       *                                                                   } */

      for (int j = 0; j < files.gl_pathc; j++)
      {			/* for each file */
        std::string file_path = files.gl_pathv[j];
        /* experiments.insert(file_path.substr(0, file_path.size()-11));
         *                         ostd::stringstream ss;
         *                                                 ss.flush();
         *                                                                         ss << experiments.size(); */
        sprintf (msg, "*** FILE Path *** %s\n", file_path.c_str ());
        std::string ext = strrchr (file_path.c_str (), '.') + 1;
        std::string filename = strrchr (file_path.c_str (), '/') + 1;
        if (database_getval ("name", filename) == "null" || 1)
        {
          std::string fileid = database_setval ("null", "name", filename);
          database_setval (fileid, "ext", ext);
          database_setval (fileid, "server", servers.at (i));
          database_setval (fileid, "location", server_ids.at (i));
          /*                          database_setval(fileid, "experiment_id", ss.str()); */
          database_setval (fileid, "file_path", file_path);
          for (int k = 0; k < server_ids.size (); k++)
          {
            database_setval (fileid, server_ids.at (k), "0");
          }
          process_file (servers.at (i), fileid, file_path);
        }
        else
        {
          std::string fileid = database_getval ("name", filename);
          database_setval (fileid, "server", servers.at (i));
          database_setval (fileid, "location", server_ids.at (i));
        }
      }
      pattern += "/*";
    }


    log_msg ("At the end of initialize\n");
    analytics ();
    return 0;
  }
  }


  int
    khan_opendir (const char *c_path, struct fuse_file_info *fi)
    {
      return 0;
    }

  bool
    find (std::string str, std::vector < std::string > arr)
    {
      for (int i = 0; i < arr.size (); i++)
      {
        if (str == arr[i])
          return true;
      }
      return false;
    }

  std::string
    str_intersect (std::string str1, std::string str2)
    {
      std::vector < std::string > vec_1 = split (str1, ":");
      std::vector < std::string > vec_2 = split (str2, ":");
      std::vector < std::string > ret;
      for (int i = 0; i < vec_1.size (); i++)
      {
        for (int j = 0; j < vec_2.size (); j++)
        {
          if ((vec_1[i] == vec_2[j]) && (!find (vec_1[i], ret)))
          {
            ret.push_back (vec_1[i]);
          }
        }
      }
      return join (ret, ":");
    }

  bool
    content_has (std::string vals, std::string val)
    {
      static std::string last_string = "";
      static std::vector < std::string > last_vector;
      std::vector < std::string > checks;

      if (last_string == vals)
      {
        checks = last_vector;
      }
      else
      {
        checks = split (vals, ":");
        last_string = vals;
        last_vector = checks;
      }

      for (int i = 0; i < checks.size (); i++)
      {
        if (checks[i] == val)
        {
          return true;
        }
      }
      return false;
    }


  int
    populate_getattr_buffer (struct stat *stbuf, std::stringstream & path)
    {
      std::string attr, val, file, more;
      std::string current = "none";
      std::string current_path = path.str ();
      void *aint = getline (path, attr, '/');
      void *vint = getline (path, val, '/');
      void *fint = getline (path, file, '/');
      void *mint = getline (path, more, '/');
      bool loop = true;
      while (loop)
      {
        /* std::cout << "top of loop" << endl << flush; */
        loop = false;
        if (aint)
        {
          std::string query = database_getval ("attrs", attr);
          /*    std::cout << "PrINT "  << "  " << attr << " " << query << endl; */
          if (query != "null")
          {
            std::string content = database_getvals (attr);
            /*      std::cout << "Query not null   " << content << endl; */
            if (vint)
            {
              /*          std::cout << "Vint is true " << endl; */
              std::cout << "Value is " << content_has (content, val) << endl;
              std::cout << "Val is  " << val << endl;
              if (content_has (content, val) || (attr == "tags"))
              {
                /*        std::cout << "Here1 " << val << endl; */
                std::string dir_content = database_getval (attr, val);
                if (current != "none")
                {
                  dir_content = str_intersect (current, dir_content);
                }
                std::string attrs_content = database_getvals ("attrs");
                if (fint)
                {
                  /*          std::cout << "fint is true " << file << endl; */
                  std::string fileid = database_getval ("name", file);
                  if (content_has (dir_content, fileid))
                  {
                    if (!mint)
                    {
                      /* /attr/val/file path */
                      std::string file_path =
                        database_getval (fileid, "file_path");
                      file_pop_stbuf (stbuf, file_path);
                      return 0;
                    }
                  }
                  else if (content_has (attrs_content, file))
                  {
                    /* repeat with aint = fint, vint = mint, etc */
                    aint = fint;
                    attr = file;
                    vint = mint;
                    val = more;
                    fint = getline (path, file, '/');
                    mint = getline (path, more, '/');
                    current = dir_content;
                    loop = true;
                  }
                }
                else
                {
                  /* /attr/val dir */
                  std::cout << "Fint is flase " << dir_content +
                    attrs_content << endl;
                  /* if(dir_content != ""){ */
                  dir_pop_stbuf (stbuf, dir_content + attrs_content);
                  return 0;
                }
                }
              }
              else
              {
                /* /attr dir */
                /* std::cout << " Here 2  " << content << endl; */
                dir_pop_stbuf (stbuf, content);
                return 0;
              }
            }
          }
          else
          {
            std::string types = database_getvals ("attrs");
            /* std::cout << " HEre 3 " << types << endl; */
            dir_pop_stbuf (stbuf, types);
            return 0;
          }
        }
        return -2;
      }

      static int
        khan_getattr (const char *c_path, struct stat *stbuf)
        {
          /* std::cout << "started get attr" << endl << flush; */
          std::string pre_processed = c_path + 1;
          if (pre_processed == ".DS_Store")
          {
            file_pop_stbuf (stbuf, pre_processed);
            return 0;
          }
          /* std::cout << "starting to resolve selectors" << endl << flush; */
          std::string after = resolve_selectors (pre_processed);
          std::stringstream path (after);
          /*std::cout << "working to pop buffer" << endl << flush;
           *         file_pop_stbuf(stbuf, "test");
           *                 int ret = 0; */
          int ret = populate_getattr_buffer (stbuf, path);
          /* std::cout << "ended get attr" << endl << flush; */
          return ret;
        }

      void
        dir_pop_buf (void *buf, fuse_fill_dir_t filler, std::string content, bool convert)
        {

          sprintf (msg, "Inside dir_pop_buf: %s\n", content.c_str ());
          log_msg (msg);

          std::vector < std::string > contents = split (content, ":");
          for (int i = 0; i < contents.size (); i++)
          {
            if (convert)
            {
              std::string filename = database_getval (contents[i].c_str (), "name");

              sprintf (msg, "dir_pop_buf loop%s\n", filename.c_str ());
              log_msg (msg);

              filler (buf, filename.c_str (), NULL, 0);
            }
            else
            {
              std::cout << "Convert is false " << endl;
              filler (buf, contents[i].c_str (), NULL, 0);
            }
          }
        }

      void
        khan_terminate (int param)
        {
          sprintf (msg, "Unmounting: %s\n", mountpoint.c_str ());
          log_msg (msg);
          unmounting (mountpoint);
          chdir ("/net/hu21/agangil3/Mediakhan/");
          log_msg ("killing...\n ");
          std::cout << "killing... " << flush << endl;
          exit (1);
        }

      /*unmount the fuse file system */
      void
        unmounting (std::string mnt_dir)
        {
          log_msg ("in umounting\n");
#ifdef APPLE
          std::string command = "umount " + mnt_dir + "\n";
#else
          std::string command = "fusermount -zu " + mnt_dir + "\n";
#endif
          if (system (command.c_str ()) < 0)
          {
            log_msg ("Could not unmount mounted directory!\n");
            log_msg (msg);
            return;
          }
          log_msg ("fusermount successful\n");
        }


      void
        populate_readdir_buffer (void *buf, fuse_fill_dir_t filler,
            std::stringstream & path)
        {

          log_msg ("Populate Read Dir Buffer\n");
          sprintf (msg, "Path is %s\n", path.str ().c_str ());
          log_msg (msg);
          std::string attr, val, file, more;
          std::string current_content = "none";
          std::string current_attrs = "none";
          std::string non_empty_content = "";
          void *aint = getline (path, attr, '/');
          void *vint = getline (path, val, '/');
          void *fint = getline (path, file, '/');
          void *mint = getline (path, more, '/');

          bool loop = true;
          while (loop)
          {
            loop = false;
            std::cout << "HO HO JUMPING!!  " << endl;
            std::string content = database_getvals ("attrs");

            std::cout << "Attrs is " << content << endl;

            if (aint)
            {
              std::cout << "Aint is true " << endl;
              std::cout << "Attr " << attr << endl;
              if (content_has (content, attr))
              {
                current_attrs += ":";
                current_attrs += attr;
                content = database_getvals (attr);

                std::cout << "Current Content   " << current_content << endl;


                if (current_content != "none")
                {
                  std::cout << "Content " << content << endl;

                  non_empty_content = "";

                  std::vector < std::string > vec_1 = split (content, ":");
                  for (int i = 0; i < vec_1.size (); ++i)
                  {
                    std::cout << "Attr " << attr << "  Val " << vec_1[i] << endl;
                    std::string dir_content = database_getval (attr, vec_1[i]);
                    if (current_content != "none")
                    {
                      dir_content =
                        str_intersect (current_content, dir_content);
                    }
                    std::cout << "Iteration " << " ** " << i << "   " <<
                      dir_content << endl;
                    if (dir_content != "")
                    {
                      non_empty_content += vec_1[i] + ":";
                    }
                  }
                }
                else
                  non_empty_content = content;


                std::cout << "Non Empty Content  " << non_empty_content << endl;

                if (vint)
                {
                  std::cout << " Content is " << content << endl;
                  std::cout << "Value is  " << content_has (content, val) << endl;
                  std::cout << "Vint is true " << endl;
                  if (content_has (content, val) || (attr == "tags"))
                  {
                    std::string dir_content = database_getval (attr, val);
                    std::cout << " ABRA  " << endl;
                    if (current_content != "none")
                    {
                      std::cout << " f sdfdsfsdf " << endl;
                      dir_content =
                        intersect (current_content, dir_content);
                    }
                    std::string attrs_content = database_getvals ("attrs");
                    if (fint)
                    {
                      std::cout << "Fint is true " << endl;
                      if (content_has (attrs_content, file))
                      {
                        /* repeat with aint = fint, vint = mint, etc */
                        aint = fint;
                        attr = file;
                        vint = mint;
                        val = more;
                        fint = getline (path, file, '/');
                        mint = getline (path, more, '/');
                        current_content = dir_content;
                        loop = true;
                      }
                    }
                    else
                    {
                      /* /attr/val dir */
                      sprintf (msg, "Else %s, %s\n\n\n\n\n\n",
                          attrs_content.c_str (),
                          current_attrs.c_str ());
                      log_msg (msg);
                      attrs_content =
                        subtract (attrs_content, current_attrs);
                      std::cout << "Dir Content  " << dir_content << endl;
                      std::cout << "Attr Content  " << attrs_content << endl;
                      dir_pop_buf (buf, filler, dir_content, true);
                      dir_pop_buf (buf, filler, attrs_content, false);
                    }
                  }
                }
                else
                {
                  /* /attr dir */
                  std::cout << "Going solo1 " << non_empty_content << endl;
                  dir_pop_buf (buf, filler, non_empty_content, false);
                }
              }
            }
            else
            {
              std::cout << " Going solo2 " << endl;
              dir_pop_buf (buf, filler, content, false);
            }
          }
          std::cout << "populate read dir end  " << endl;
        }
