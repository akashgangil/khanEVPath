#include <string>
#include <stdio.h>
#include <vector>
#include <iostream>
#include <errno.h>
#include <glob.h>
#include <string.h>
#include <set>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

#include <boost/log/trivial.hpp>

#include "khan.h"
#include "data_analytics.h"
#include "fileprocessor.h"
#include "database.h"
#include "utils.h"


std::string primary_attribute = "";

std::string mountpoint;

  void *
initializing_khan (void *mnt_dir, std::vector < std::string> servers, std::vector < std::string > server_ids, int port)
{

  BOOST_LOG_TRIVIAL(info) << "Initializing Khan";
  
  /* Opening root directory and creating if not present */
  BOOST_LOG_TRIVIAL(debug) << "Khan Root 0 is " << servers[0];
  
  if (NULL == opendir (servers.at (0).c_str ()))
  {
    BOOST_LOG_TRIVIAL(error) << "Error message on opening directory" << strerror(errno);
    BOOST_LOG_TRIVIAL(error) << "Root directory might not exist.. Creating";
    std::string command = "mkdir " + servers.at (0);
    if (system (command.c_str ()) < 0)
    {
      BOOST_LOG_TRIVIAL(fatal) << "Unable to create storage directory... Aborting";
      exit (1);
    }
  }
  else
  {
    BOOST_LOG_TRIVIAL(info) << "Directory opened successfully";
  }

  init_database(port);

  /* check if we've loaded metadata before */
  std::string output = database_getval ("setup", "value");
  
  if (output.compare ("true") == 0)
  {
    BOOST_LOG_TRIVIAL(info) << "Database was previously initialized";
    tot_time +=
      (stop.tv_sec - start.tv_sec) + (stop.tv_nsec -
          start.tv_nsec) / BILLION;
    return 0;			/* setup has happened before */
  }

  /* if we have not setup, do so now */
  BOOST_LOG_TRIVIAL(info) << "Set database setup value to true and setup the database";
  database_setval ("setup", "value", "true");

  /* load metadata associatons */
  for (unsigned i = 0; i < servers.size (); i++)
  {
    process_transducers (servers.at (i));
  }

  /* load metadata for each file on each server */
  std::string types = database_getval ("allfiles", "types");
  
  BOOST_LOG_TRIVIAL(info) << "Types to look for" << types;

  //analytics ();
    
  BOOST_LOG_TRIVIAL(info) << "Khan Initialized";
    
  return 0;
}


  int
    khan_opendir (const char *c_path, struct fuse_file_info *fi)
    {
      return 0;
    }

  bool
    find (std::string str, std::vector < std::string > arr)
    {
      for (unsigned i = 0; i < arr.size (); i++)
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
      for (unsigned i = 0; i < vec_1.size (); i++)
      {
        for (unsigned j = 0; j < vec_2.size (); j++)
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

      for (unsigned i = 0; i < checks.size (); i++)
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
        /* std::cout << "top of loop" << std::endl << flush; */
        loop = false;
        if (aint)
        {
          std::string query = database_getval ("attrs", attr);
          /*    std::cout << "PrINT "  << "  " << attr << " " << query << std::endl; */
          if (query != "null")
          {
            std::string content = database_getvals (attr);
            /*      std::cout << "Query not null   " << content << std::endl; */
            if (vint)
            {
              /*          std::cout << "Vint is true " << std::endl; */
              std::cout << "Value is " << content_has (content, val) << std::endl;
              std::cout << "Val is  " << val << std::endl;
              if (content_has (content, val) || (attr == "tags"))
              {
                /*        std::cout << "Here1 " << val << std::endl; */
                std::string dir_content = database_getval (attr, val);
                if (current != "none")
                {
                  dir_content = str_intersect (current, dir_content);
                }
                std::string attrs_content = database_getvals ("attrs");
                if (fint)
                {
                  /*          std::cout << "fint is true " << file << std::endl; */
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
                    attrs_content << std::endl;
                  /* if(dir_content != ""){ */
                  dir_pop_stbuf (stbuf, dir_content + attrs_content);
                  return 0;
                }
                }
              }
              else
              {
                /* /attr dir */
                /* std::cout << " Here 2  " << content << std::endl; */
                dir_pop_stbuf (stbuf, content);
                return 0;
              }
            }
          }
          else
          {
            std::string types = database_getvals ("attrs");
            /* std::cout << " HEre 3 " << types << std::endl; */
            dir_pop_stbuf (stbuf, types);
            return 0;
          }
        }
        return -2;
      }

      int
        khan_getattr (const char *c_path, struct stat *stbuf)
        {
          /* std::cout << "started get attr" << std::endl << flush; */
          std::string pre_processed = c_path + 1;
          if (pre_processed == ".DS_Store")
          {
            file_pop_stbuf (stbuf, pre_processed);
            return 0;
          }
          /* std::cout << "starting to resolve selectors" << std::endl << flush; */
          std::string after = resolve_selectors (pre_processed);
          std::stringstream path (after);
          /*std::cout << "working to pop buffer" << std::endl << flush;
           *         file_pop_stbuf(stbuf, "test");
           *                 int ret = 0; */
          int ret = populate_getattr_buffer (stbuf, path);
          /* std::cout << "ended get attr" << std::endl << flush; */
          return ret;
        }

      void
        dir_pop_buf (void *buf, fuse_fill_dir_t filler, std::string content, bool convert)
        {

          BOOST_LOG_TRIVIAL(info) << "Inside dir_pop_buf " << content;

          std::vector < std::string > contents = split (content, ":");
          for (unsigned i = 0; i < contents.size (); i++)
          {
            if (convert)
            {
              std::string filename = database_getval (contents[i].c_str (), "name");
              filler (buf, filename.c_str (), NULL, 0);
            }
            else
            {
              filler (buf, contents[i].c_str (), NULL, 0);
            }
          }
        }

      /*unmount the fuse file system */
      void
        unmounting (std::string mnt_dir)
        {
          
          BOOST_LOG_TRIVIAL(debug) << "In unmounting";
#ifdef APPLE
          std::string command = "umount " + mnt_dir + "\n";
#else
          std::string command = "fusermount -zu " + mnt_dir + "\n";
#endif
          if (system (command.c_str ()) < 0)
          {
            BOOST_LOG_TRIVIAL(error) << "Could not unmount the specfied directory";
            return;
          }
          
          BOOST_LOG_TRIVIAL(info) << "fuserunmount successful";
        }


      void
        populate_readdir_buffer (void *buf, fuse_fill_dir_t filler,
            std::stringstream & path)
        {

          BOOST_LOG_TRIVIAL(info) << "Populate read directory buffer";

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
            std::cout << "HO HO JUMPING!!  " << std::endl;
            std::string content = database_getvals ("attrs");

            std::cout << "Attrs is " << content << std::endl;

            if (aint)
            {
              std::cout << "Aint is true " << std::endl;
              std::cout << "Attr " << attr << std::endl;
              if (content_has (content, attr))
              {
                current_attrs += ":";
                current_attrs += attr;
                content = database_getvals (attr);

                std::cout << "Current Content   " << current_content << std::endl;


                if (current_content != "none")
                {
                  std::cout << "Content " << content << std::endl;

                  non_empty_content = "";

                  std::vector < std::string > vec_1 = split (content, ":");
                  for (unsigned i = 0; i < vec_1.size (); ++i)
                  {
                    std::cout << "Attr " << attr << "  Val " << vec_1[i] << std::endl;
                    std::string dir_content = database_getval (attr, vec_1[i]);
                    if (current_content != "none")
                    {
                      dir_content =
                        str_intersect (current_content, dir_content);
                    }
                    std::cout << "Iteration " << " ** " << i << "   " <<
                      dir_content << std::endl;
                    if (dir_content != "")
                    {
                      non_empty_content += vec_1[i] + ":";
                    }
                  }
                }
                else
                  non_empty_content = content;


                std::cout << "Non Empty Content  " << non_empty_content << std::endl;

                if (vint)
                {
                  std::cout << " Content is " << content << std::endl;
                  std::cout << "Value is  " << content_has (content, val) << std::endl;
                  std::cout << "Vint is true " << std::endl;
                  if (content_has (content, val) || (attr == "tags"))
                  {
                    std::string dir_content = database_getval (attr, val);
                    std::cout << " ABRA  " << std::endl;
                    if (current_content != "none")
                    {
                      std::cout << " f sdfdsfsdf " << std::endl;
                      dir_content =
                        intersect (current_content, dir_content);
                    }
                    std::string attrs_content = database_getvals ("attrs");
                    if (fint)
                    {
                      std::cout << "Fint is true " << std::endl;
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
      
                      BOOST_LOG_TRIVIAL(info) << "Else " << attrs_content <<" " << current_attrs;
                      attrs_content =
                        subtract (attrs_content, current_attrs);
                      std::cout << "Dir Content  " << dir_content << std::endl;
                      std::cout << "Attr Content  " << attrs_content << std::endl;
                      dir_pop_buf (buf, filler, dir_content, true);
                      dir_pop_buf (buf, filler, attrs_content, false);
                    }
                  }
                }
                else
                {
                  /* /attr dir */
                  std::cout << "Going solo1 " << non_empty_content << std::endl;
                  dir_pop_buf (buf, filler, non_empty_content, false);
                }
              }
            }
            else
            {
              std::cout << " Going solo2 " << std::endl;
              dir_pop_buf (buf, filler, content, false);
            }
          }
          std::cout << "populate read dir end  " << std::endl;
        }
