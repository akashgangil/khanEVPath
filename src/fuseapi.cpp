
#include <stdio.h>
#include <string.h>
#include <fstream>

#include "params.h"

#include <fuse.h>
#include <fuse/fuse_opt.h>
#include <fuse/fuse_lowlevel.h>

#include <boost/log/trivial.hpp>

#include <errno.h>

#include "khan.h"
#include "fuseapi.h"
#include "fuse_helper.h"
#include "database.h"
#include "utils.h"
#include "localizations.h"
#include "fileprocessor.h"

struct fuse_operations khan_ops;

void
xmp_initialize ()
{

  khan_ops.getattr = khan_getattr;
  khan_ops.init = khan_init;
  khan_ops.access = xmp_access;
  khan_ops.readlink = xmp_readlink;
  khan_ops.readdir = xmp_readdir;
  khan_ops.mknod = xmp_mknod;
  khan_ops.mkdir = xmp_mkdir;
  khan_ops.symlink = xmp_symlink;
  khan_ops.unlink = xmp_unlink;
  khan_ops.rmdir = xmp_rmdir;
  khan_ops.rename = xmp_rename;
  khan_ops.link = xmp_link;
  khan_ops.chmod = xmp_chmod;
  khan_ops.chown = xmp_chown;
  khan_ops.truncate = xmp_truncate;
  khan_ops.create = khan_create;
  khan_ops.utimens = xmp_utimens;
  khan_ops.open = khan_open;
  khan_ops.read = xmp_read;
  khan_ops.write = xmp_write;
  khan_ops.statfs = xmp_statfs;
  khan_ops.release = xmp_release;
  khan_ops.fsync = xmp_fsync;
  khan_ops.opendir = khan_opendir;
  khan_ops.flush = khan_flush;
  khan_ops.getxattr = xmp_getxattr;
#ifdef APPLE
  khan_ops.setxattr = xmp_setxattr;
  khan_ops.listxattr = xmp_listxattr;
  khan_ops.removexattr = xmp_removexattr;
  khan_ops.setvolname = xmp_setvolname;
  khan_ops.exchange = xmp_exchange;
  khan_ops.getxtimes = xmp_getxtimes;
  khan_ops.setbkuptime = xmp_setbkuptime;
  khan_ops.setchgtime = xmp_setchgtime;
  khan_ops.setcrtime = xmp_setcrtime;
  khan_ops.chflags = xmp_chflags;
  khan_ops.setattr_x = xmp_setattr_x;
  khan_ops.fsetattr_x = xmp_fsetattr_x;
#endif
}

  void *
khan_init (struct fuse_conn_info *conn)
{

  BOOST_LOG_TRIVIAL(info) << "khan_init called";
  
  if (chdir (servers.at (0).c_str ()) < 0)
  {
    BOOST_LOG_TRIVIAL(error) << "Could not change directory " << "Errno " << strerror(errno);
    perror (servers.at (0).c_str ());
  }
  
  BOOST_LOG_TRIVIAL(info) << "khan_int ends";
  return KHAN_DATA;
}

  int
khan_flush (const char *path, struct fuse_file_info *info)
{
  
  BOOST_LOG_TRIVIAL(info) << "Khan Flush";
  
  std::string filename = basename (strdup (path));
  std::string fileid = database_getval ("name", filename);
  std::string server = database_getval (fileid, "server");
  std::string file_path = database_getval (fileid, "file_path");

  process_file (server, fileid, file_path);
  return 0;
}

  int
khan_open (const char *path, struct fuse_file_info *fi)
{

  BOOST_LOG_TRIVIAL(info) << "Khan Open Directory";
  
  int retstat = 0;
  int fd;
  path = basename (strdup (path));
  std::string fileid = database_getval ("name", path);
  
  /* Get server  */
  std::string server = database_getval (fileid, "server");
  if (server == "cloud")
  {
    /* std::cout << "looking at cloud" << std::endl << flush;  */
    std::string long_path = "/tmp/";
    long_path += path;
  }
  return 0;
}

  int
khan_create (const char *path, mode_t mode, struct fuse_file_info *fi)
{
  create_calls++;

  BOOST_LOG_TRIVIAL(info) << "Khan xmp_create";
  
  std::string fileid = database_getval ("name", basename (strdup (path)));
  if (strcmp (fileid.c_str (), "null") == 0)
  {
    fileid = database_setval ("null", "name", basename (strdup (path)));
    database_setval (fileid, "server", servers.at (0));
    std::string ext = strrchr (basename (strdup (path)), '.') + 1;
    database_setval (fileid, "ext", ext);
  }
  std::string server = database_getval (fileid, "server");

  process_file (server, fileid, "");

  map_path (resolve_selectors (path), fileid);

  return 0;
}

  static int
xmp_access (const char *path, int mask)
{

  BOOST_LOG_TRIVIAL(info) << "Khan Access " << path;
  char *path_copy = strdup (path);
  if (strcmp (path, "/") == 0)
  {
    
    BOOST_LOG_TRIVIAL(info) << "At root " ;
    return 0;
  }
  BOOST_LOG_TRIVIAL(info) << "At root " ;
  return 0;

  std::string dirs = database_getval ("alldirs", "paths");
  std::string temptok = "";
  std::stringstream dd (dirs);
  while (getline (dd, temptok, ':'))
  {
    if (strcmp (temptok.c_str (), path) == 0)
    {
      return 0;
    }
  }

  int c = 0;
  for (int i = 0; path[i] != '\0'; i++)
  {
    if (path[i] == '/')
      c++;
  }

  /*decompose path */
  std::stringstream ss0 (path + 1);
  std::string type, attr, val, file, more;
  void *tint = getline (ss0, type, '/');
  void *fint = getline (ss0, file, '/');
  void *mint = getline (ss0, more, '/');
  int reta = 0;

  /*check for filetype */
  if (tint)
  {
    std::string types = database_getval ("allfiles", "types");
    std::stringstream ss (types.c_str ());
    std::string token;
    while (getline (ss, token, ':'))
    {
      if (strcmp (type.c_str (), token.c_str ()) == 0)
      {
        reta = 1;
      }
    }
    int found = 0;

    do
    {
      /* get attr and val */
      found = 0;
      void *aint = fint;
      std::string attr = file;
      void *vint = mint;
      std::string val = more;
      fint = getline (ss0, file, '/');
      mint = getline (ss0, more, '/');

      /* check for attr */
      if (reta && aint)
      {
        std::string attrs = database_getval (type, "attrs");
        std::stringstream ss3 (attrs.c_str ());
        reta = 0;
        while (getline (ss3, token, ':'))
        {
          if (strcmp (attr.c_str (), token.c_str ()) == 0)
          {
            reta = 1;
          }
        }

        /* check for val */
        if (reta && vint)
        {
          std::cout << val << std::endl;
          if (strcmp (attr.c_str (), ("all_" + type + "s").c_str ())
              == 0)
          {
            clock_gettime (CLOCK_REALTIME, &stop);
            time_spent =
              (stop.tv_sec - start.tv_sec) + (stop.tv_nsec -
                  start.tv_nsec) /
              BILLION;
            tot_time += time_spent;;
            access_avg_time =
              (access_avg_time * (access_calls - 1) +
               time_spent) / access_calls;
            return 0;
          }
          std::string vals = database_getvals (attr);
          std::stringstream ss4 (vals.c_str ());
          reta = 0;
          while (getline (ss4, token, ':'))
          {
            std::cout << val << token << std::endl;
            if (strcmp (val.c_str (), token.c_str ()) == 0)
            {
              reta = 1;
            }
          }

          /* check for file */
          if (reta && fint)
          {
            std::cout << file << std::endl;
            std::string files = database_getval (attr, val);
            std::stringstream ss4 (files.c_str ());
            if (!mint)
            {
              reta = 0;
              while (getline (ss4, token, ':'))
              {
                token = database_getval (token, "name");
                if (strcmp (file.c_str (), token.c_str ()) == 0)
                {
                  reta = 1;
                }
              }
              std::stringstream ss5 (attrs.c_str ());
              while (getline (ss5, token, ':'))
              {
                if (strcmp (file.c_str (), token.c_str ()) == 0)
                {
                  reta = 1;
                }
              }
            }
            else
            {
              found = 1;
            }
          }
        }
      }
    }
    while (found);
  }

  if (reta && !getline (ss0, val, '/'))
  {
    return 0;
  }
  path = append_path (path);
  int ret = access (path, mask);
  return ret;
}


  static int
xmp_mknod (const char *path, mode_t mode, dev_t rdev)
{
  path = append_path2 (basename (strdup (path)));

  BOOST_LOG_TRIVIAL(info) << "In xmp_mknod Path = " << path  ;
  
  int res;
  if (S_ISFIFO (mode))
    res = mkfifo (path, mode);
  else
    res = mknod (path, mode, rdev);
  if (res == -1)
  {
    fprintf (stderr, "\nmknod error \n");
    return -errno;
  }
  return 0;
}

  static int
xmp_mkdir (const char *path, mode_t mode)
{
  struct timespec mkdir_start, mkdir_stop;
  
  BOOST_LOG_TRIVIAL(info) << "Khan mkdir " << path;
  
  std::string strpath = path;
  if (strpath.find ("localize") != std::string::npos)
  {
    if (strpath.find ("usage") != std::string::npos)
    {
      usage_localize ();
    }
    else
    {
      std::string filename = "winter.mp3";
      std::string fileid = database_getval ("name", filename);
      std::string location = get_location (fileid);
      std::string server = database_getval (fileid, "server");
      /*if not current */
      if (location.compare (server) != 0)
      {
        /*  move to new location */
        database_setval (fileid, "server", location);
        std::string from = server + "/" + filename;
        std::string to = location + "/" + filename;
        std::string command = "mv " + from + " " + to;
        FILE *stream = popen (command.c_str (), "r");
        pclose (stream);
      }
    }
    return -1;
  }
  if (strpath.find ("stats") != std::string::npos)
  {
    /* print stats and reset */
    std::ofstream stfile;
    stfile << "TOT TIME    :" << tot_time << std::endl;
    stfile << "Vold Calls   :" << vold_calls << std::endl;
    stfile << "     Avg Time:" << vold_avg_time << std::endl;
    stfile << "Readdir Calls:" << readdir_calls << std::endl;
    stfile << "     Avg Time:" << readdir_avg_time << std::endl;
    stfile << "Access Calls :" << access_calls << std::endl;
    stfile << "     Avg Time:" << access_avg_time << std::endl;
    stfile << "Read Calls   :" << read_calls << std::endl;
    stfile << "     Avg Time:" << read_avg_time << std::endl;
    stfile << "Getattr Calls:" << getattr_calls << std::endl;
    stfile << "     Avg Time:" << getattr_avg_time << std::endl;
    stfile << "Write Calls  :" << write_calls << std::endl;
    stfile << "     Avg Time:" << write_avg_time << std::endl;
    stfile << "Create Calls :" << create_calls << std::endl;
    stfile << "     Avg Time:" << create_avg_time << std::endl;
    stfile << "Rename Calls :" << rename_calls << std::endl;
    stfile << "     Avg Time:" << rename_avg_time << std::endl;
    stfile.close ();
    vold_calls = 0;
    readdir_calls = 0;
    access_calls = 0;
    getattr_calls = 0;
    read_calls = 0;
    write_calls = 0;
    create_calls = 0;
    rename_calls = 0;
    tot_time = 0;
    vold_avg_time = 0;
    readdir_avg_time = 0;
    access_avg_time = 0;
    getattr_avg_time = 0;
    read_avg_time = 0;
    write_avg_time = 0;
    create_avg_time = 0;
    rename_avg_time = 0;
    return -1;
  }

  BOOST_LOG_TRIVIAL(info) << "xmp_mkdir for path = " << path;
  struct stat *st;
  if (khan_getattr (path, st) < 0)
  {
    /*add path */
    database_setval ("alldirs", "paths", path);
    /*and break into attr/val pair and add to vold */
  }
  else
  {
    BOOST_LOG_TRIVIAL(info) << "Directory exists";
  }
  return 0;
}


  static int
xmp_readlink (const char *path, char *buf, size_t size)
{
  
  BOOST_LOG_TRIVIAL(info) << "xmp_readlink Path = " << path;
  
  /* TODO: handle in vold somehow */
  int res = -1;
  path = append_path2 (basename (strdup (path)));
  /* res = readlink(path, buf, size - 1); */
  if (res == -1)
    return -errno;
  buf[res] = '\0';
  return 0;
}

  static int
xmp_unlink (const char *path)
{

  BOOST_LOG_TRIVIAL(info) << "xmp_unlink, PATH = " << path;
  /* TODO: handle in vold somehow */
  int res;
  std::string fileid = database_getval ("name", basename (strdup (path)));

  std::string fromext = database_getval (fileid, "ext");
  std::string file = append_path2 (basename (strdup (path)));
  std::string attrs = database_getval (fromext, "attrs");
  database_remove_val (fileid, "attrs", "all_" + fromext + "s");
  std::string token = "";
  std::stringstream ss2 (attrs.c_str ());
  while (getline (ss2, token, ':'))
  {
    if (strcmp (token.c_str (), "null") != 0)
    {
      std::string cmd = database_getval (token + "gen", "command");
      std::string msg2 = (cmd + " " + file).c_str ();
      FILE *stream = popen (msg2.c_str (), "r");
      char msg[200];
      if (fgets (msg, 200, stream) != 0)
      {
        database_remove_val (fileid, token, msg);
      }
      pclose (stream);
    }
  }

  path = append_path2 (basename (strdup (path)));
  res = unlink (path);
  if (res == -1)
    return -errno;
  return 0;
}


  static int
xmp_rmdir (const char *path)
{
  BOOST_LOG_TRIVIAL(info) << "xmp_rmdir PATH = " << path;
  /*if hardcoded, just remove */
  database_remove_val ("alldirs", "paths", path);

  /*if exists
   *get contained files
   *get attrs+vals from path
   *unset files attrs
   *if entire attr, remove attr*/

  return 0;
}

  static int
xmp_symlink (const char *from, const char *to)
{
  /*TODO: handle in vold somehow */
  int res = -1;
  from = append_path2 (basename (strdup (from)));
  to = append_path2 (basename (strdup (to)));
  
  BOOST_LOG_TRIVIAL(info) << "xmp_symlink From " << from << " to " << to;
  
  if (res == -1)
    return -errno;
  return 0;
}

  static int
xmp_link (const char *from, const char *to)
{
  /*TODO:handle in vold somehow... */
  int retstat = 0;
  from = append_path2 (basename (strdup (from)));
  to = append_path2 (basename (strdup (to)));
  BOOST_LOG_TRIVIAL(info) << "xmp_link. From " << from << " To: " << to;
  retstat = link (from, to);
  return retstat;
}

  static int
xmp_chmod (const char *path, mode_t mode)
{
  int res;
  path = append_path2 (basename (strdup (path)));
  
  BOOST_LOG_TRIVIAL(info) << "xmp_chmod Path = " << path;
  
  res = chmod (path, mode);
#ifdef APPLE
  res = chmod (path, mode);
#else
  res = chmod (path, mode);
#endif
  if (res == -1)
    return -errno;
  return 0;
}

  static int
xmp_chown (const char *path, uid_t uid, gid_t gid)
{
  int res;
  path = append_path2 (basename (strdup (path)));
  
  BOOST_LOG_TRIVIAL(info) << "xmp_chown, PATH = " << path;
  
  res = lchown (path, uid, gid);
  if (res == -1)
    return -errno;
  return 0;
}

  static int
xmp_truncate (const char *path, off_t size)
{
  /*update for vold? */
  int res;
  path++;
  BOOST_LOG_TRIVIAL(info) << "xmp_truncate, PATH = " << path;
  res = truncate (path, size);
  if (res == -1)
    return -errno;
  return 0;
}

  static int
xmp_utimens (const char *path, const struct timespec ts[2])
{
  int res;
  struct timeval tv[2];
  path = append_path2 (basename (strdup (path)));
  
  BOOST_LOG_TRIVIAL(info) << "xmp_utimens, PATH = " << path;

  tv[0].tv_sec = ts[0].tv_sec;
  tv[0].tv_usec = ts[0].tv_nsec / 1000;
  tv[1].tv_sec = ts[1].tv_sec;
  tv[1].tv_usec = ts[1].tv_nsec / 1000;
  res = utimes (path, tv);
  if (res == -1)
    return -errno;
  return 0;
}

  static int
xmp_read (const char *path, char *buf, size_t size, off_t offset,
    struct fuse_file_info *fi)
{
  int res = 0;

  path = append_path2 (basename (strdup (path)));
  BOOST_LOG_TRIVIAL(info) << "xmp_read , PATH = " << path;

  FILE *thefile = fopen (path, "r");
  if (thefile != NULL)
  {
    fseek (thefile, offset, SEEK_SET);
    res = fread (buf, 1, size, thefile);
    if (res == -1)
      res = -errno;
    fclose (thefile);
  }
  else
  {
    res = -errno;
  }
  return res;
}

  static int
xmp_write (const char *path, const char *buf, size_t size, off_t offset,
    struct fuse_file_info *fi)
{
  int fd;
  int res;

  path = append_path2 (basename (strdup (path)));
  BOOST_LOG_TRIVIAL(info) << "xmp_write, PATH = " << path;
  (void) fi;
  fd = open (path, O_WRONLY);
  if (fd == -1)
  {
    return errno;
  }
  res = pwrite (fd, buf, size, offset);
  if (res == -1)
    res = errno;
  close (fd);
  return res;
}

  static int
xmp_statfs (const char *path, struct statvfs *stbuf)
{
  /* Pass the call through to the underlying system which has the media. */

  BOOST_LOG_TRIVIAL(info) << "xmp_statfs, PATH = " << path;
  int res = statvfs (path, stbuf);
  if (res != 0)
  {
    //fprintf (log, "statfs error for %s\n", path);
    return errno;
  }
  return 0;
}

  static int
xmp_release (const char *path, struct fuse_file_info *fi)
{
  /* Just a stub. This method is optional and can safely be left unimplemented. */
  BOOST_LOG_TRIVIAL(debug) << "xmp_release, PATH = " << path;
  return 0;
}

  static int
xmp_fsync (const char *path, int isdatasync, struct fuse_file_info *fi)
{
  /* Just a stub. This method is optional and can safely be left unimplemented. */
  BOOST_LOG_TRIVIAL(info) << "xmp_fsync, PATH = " << path;
  return 0;
}

  static int
xmp_rename (const char *from, const char *to)
{
  
  BOOST_LOG_TRIVIAL(info) << "xmp_rename, From: " << from << " To: " << to;
  double start_time = 0;
  struct timeval start_tv;
  gettimeofday (&start_tv, NULL);
  start_time = start_tv.tv_sec;
  start_time += (start_tv.tv_usec / 1000000.0);
  std::string src = basename (strdup (from));
  std::string dst = basename (strdup (to));
  std::string fileid = database_getval ("name", src);
  database_remove_val (fileid, "name", src);
  database_setval (fileid, "name", dst);
  std::string orig_path = append_path2 (src);
  std::string orig_loc = database_getval (fileid, "location");
  map_path (resolve_selectors (to), fileid);
  std::string new_path = append_path2 (dst);
  std::string new_loc = database_getval (fileid, "location");
  if (new_loc != orig_loc)
  {
    if (new_loc == "google_music")
    {
      /* upload */
      /* cloud_upload(orig_path); */
    }
    else if (orig_loc == "google_music")
    {
      /* download */
      /* cloud_download(src, new_path); */
    }
    else
    {
      /* file system rename */
      rename (orig_path.c_str (), new_path.c_str ());
    }
  }
  double rename_time = 0;
  struct timeval end_tv;
  gettimeofday (&end_tv, NULL);
  rename_time = end_tv.tv_sec - start_tv.tv_sec;
  rename_time += (end_tv.tv_usec - start_tv.tv_usec) / 1000000.0;
  return 0;
}

  static int
xmp_readdir (const char *c_path, void *buf, fuse_fill_dir_t filler,
    off_t offset, struct fuse_file_info *fi)
{

  BOOST_LOG_TRIVIAL(info) << "xmp_readdir PATH = " << c_path;
  filler (buf, ".", NULL, 0);
  filler (buf, "..", NULL, 0);
  std::string pre_processed = c_path + 1;
  std::string after = resolve_selectors (pre_processed);
  std::stringstream path (after);
  populate_readdir_buffer (buf, filler, path);
  return 0;
}


#ifdef APPLE
  static int
xmp_setxattr (const char *path, const char *name, const char *value,
    size_t size, int flags, uint32_t param)
{
#else
  static int
    xmp_setxattr (const char *path, const char *name, const char *value,
        size_t size, int flags)
    {
#endif
      std::string attr = bin2hex (value, size);

      BOOST_LOG_TRIVIAL(info) << "setxattr call Path: " << path << " Name: " << name << " Value: " << value;
      
      std::string xpath = "xattr:";
      xpath += path;
      redis_setval (xpath, name, attr.c_str ());
      
      BOOST_LOG_TRIVIAL(info) << "setxattr call " << xpath << name << attr;
      
      return 0;
    }

#ifdef APPLE
  static int
    xmp_getxattr (const char *path, const char *name, char *value, size_t size,
        uint32_t param)
    {
#else
      static int
        xmp_getxattr (const char *path, const char *name, char *value, size_t size)
        {
#endif
          fprintf (stderr, "getxattr call\n %s, %s, %s\n", path, name, value);
          std::string xpath = "xattr:";
          xpath += path;
          std::string db_val = redis_getval (xpath, name);
      
          BOOST_LOG_TRIVIAL(info) << "getxattr call " << "Path: " << xpath << "Name: " << name
                      << "Value: " << db_val << "Size: " << size;
          
          if (db_val != "null")
          {
            db_val = hex2bin (db_val);
            if (value == NULL)
            {
              errno = 0;
              return db_val.length ();
            }
            memcpy (value, db_val.c_str (), size);
            size_t num = snprintf (value, size, "%s", db_val.c_str ());
          
            BOOST_LOG_TRIVIAL(debug) << "Returned String: " << value << "Count: " << num;
            
            errno = 0;
            return size;
          }
          errno = 1;
          return ENODATA;
        }

      static int
        xmp_listxattr (const char *path, char *list, size_t size)
        {

          BOOST_LOG_TRIVIAL(debug) << "listxattr call " << path << list;
          std::string xpath = "xattr:";
          xpath += path;
          std::string attrs = database_getvals (xpath);
          char *mal = strdup (attrs.c_str ());
          int count = 1;
          int str_size = strlen (mal);
          for (int i = 0; i < str_size; i++)
          {
            if (mal[i] == ':')
            {
              mal[i] = '\0';
              count += 1;
            }
          }
          if (list == NULL)
          {
            BOOST_LOG_TRIVIAL(debug) << "returning " << str_size;
            return str_size;
          }
          snprintf (list, size, "%s", attrs.c_str ());
          
          BOOST_LOG_TRIVIAL(debug) << "Returning " << list << count;
          
          return count;
        }

      static int
        xmp_removexattr (const char *path, const char *name)
        {
          fprintf (stderr, "removexattr call\n %s, %s\n\n", path, name);
          return 0;
        }

#ifdef APPLE

      static int
        xmp_setvolname (const char *param)
        {
          fprintf (stderr, "apple function called setvolname\n");
          return 0;
        }

      static int
        xmp_exchange (const char *param1, const char *param2, unsigned long param3)
        {
          fprintf (stderr, "apple function called exchange\n");
          return 0;
        }

      static int
        xmp_getxtimes (const char *param1, struct timespec *param2,
            struct timespec *param3)
        {
          fprintf (stderr, "apple function called xtimes\n");
          return 0;
        }

      static int
        xmp_setbkuptime (const char *param1, const struct timespec *param2)
        {
          fprintf (stderr, "apple function called setbkuptimes\n");
          return 0;
        }

      static int
        xmp_setchgtime (const char *param1, const struct timespec *param2)
        {
          fprintf (stderr, "apple function called setchgtimes\n");
          return 0;
        }

      static int
        xmp_setcrtime (const char *param1, const struct timespec *param2)
        {
          fprintf (stderr, "apple function called setcrtimes\n");
          return 0;
        }

      static int
        xmp_chflags (const char *param1, uint32_t param2)
        {
          fprintf (stderr, "apple function called chflags\n");
          return 0;
        }

      static int
        xmp_setattr_x (const char *param1, struct setattr_x *param2)
        {
          fprintf (stderr, "apple function called setattr_x\n");
          return 0;
        }

      static int
        xmp_fsetattr_x (const char *param1, struct setattr_x *param2,
            struct fuse_file_info *param3)
        {
          fprintf (stderr, "apple function called fsetattr_x\n");
          return 0;
        }
#endif
