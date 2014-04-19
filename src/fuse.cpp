
static struct fuse_operations khan_ops;

static void xmp_initialize(){

  khan_ops.getattr  = khan_getattr;
  khan_ops.init     = khan_init;
  khan_ops.access    = xmp_access;
  khan_ops.readlink  = xmp_readlink;
  khan_ops.readdir  = xmp_readdir;
  khan_ops.mknod    = xmp_mknod;
  khan_ops.mkdir    = xmp_mkdir;
  khan_ops.symlink  = xmp_symlink;
  khan_ops.unlink    = xmp_unlink;
  khan_ops.rmdir    = xmp_rmdir;
  khan_ops.rename    = xmp_rename;
  khan_ops.link    = xmp_link;
  khan_ops.chmod    = xmp_chmod;
  khan_ops.chown    = xmp_chown;
  khan_ops.truncate  = xmp_truncate;
  khan_ops.create   = khan_create;
  khan_ops.utimens  = xmp_utimens;
  khan_ops.open    = khan_open;
  khan_ops.read    = xmp_read;
  khan_ops.write    = xmp_write;
  khan_ops.statfs    = xmp_statfs;
  khan_ops.release  = xmp_release;
  khan_ops.fsync    = xmp_fsync;
  khan_ops.opendir  = khan_opendir;
  khan_ops.flush    = khan_flush;
  khan_ops.getxattr  = xmp_getxattr;
#ifdef APPLE
  khan_ops.setxattr  = xmp_setxattr;
  khan_ops.listxattr  = xmp_listxattr;
  khan_ops.removexattr  = xmp_removexattr;
  khan_ops.setvolname     = xmp_setvolname;
  khan_ops.exchange       = xmp_exchange;
  khan_ops.getxtimes      = xmp_getxtimes;
  khan_ops.setbkuptime    = xmp_setbkuptime;
  khan_ops.setchgtime     = xmp_setchgtime;
  khan_ops.setcrtime      = xmp_setcrtime;
  khan_ops.chflags        = xmp_chflags;
  khan_ops.setattr_x      = xmp_setattr_x;
  khan_ops.fsetattr_x     = xmp_fsetattr_x;
#endif
}

void *khan_init(struct fuse_conn_info *conn) {

  log_msg("khan_init() called!\n");
  sprintf(msg,"khan_root is : %s\n",servers.at(0).c_str());
  log_msg(msg);
  if(chdir(servers.at(0).c_str())<0) {
    sprintf(msg,"could not change directory ,errno %s\n",strerror(errno)); 
    log_msg(msg);
    perror(servers.at(0).c_str());
  }
  sprintf(msg,"AT THE END OF INIT\n"); 
  log_msg(msg);
  return KHAN_DATA;
}

int khan_flush (const char * path, struct fuse_file_info * info ) {

  sprintf(msg, "Khan flush: %s\n", path);
  log_msg(msg);
  string filename = basename(strdup(path));
  string fileid=database_getval("name",filename);
  string server=database_getval(fileid,"server");
  string file_path = database_getval(fileid, "file_path");

  process_file(server, fileid, file_path);
  return 0;
}


static int xmp_access(const char *path, int mask)
{

  sprintf(msg, "Khan Access: %s", path);
  log_msg(msg);
  char *path_copy=strdup(path);
  if(strcmp(path,"/")==0) {
    log_msg("at root");
    return 0;
  }
  /* if(strcmp(path,"/")==0) { */
  log_msg("at root\n");
  return 0;
  /*  } */

  string dirs=database_getval("alldirs","paths");
  string temptok="";
  stringstream dd(dirs);
  while(getline(dd,temptok,':')){
    if(strcmp(temptok.c_str(),path)==0){
      return 0;
    }
  }

  int c=0;
  for(int i=0; path[i]!='\0'; i++){
    if(path[i]=='/') c++;
  }

  /*decompose path*/
  stringstream ss0(path+1);
  string type, attr, val, file, more;
  void* tint=getline(ss0, type, '/');
  void* fint=getline(ss0, file, '/');
  void* mint=getline(ss0, more, '/');
  int reta=0;

  /*check for filetype*/
  if(tint){
    string types = database_getval("allfiles","types");
    stringstream ss(types.c_str());
    string token;
    while(getline(ss,token,':')){
      if(strcmp(type.c_str(),token.c_str())==0){
        reta=1;
      }
    }
    int found=0;

    do{
      /* get attr and val */
      found=0;
      void *aint=fint;
      string attr=file;
      void *vint=mint;
      string val=more;
      fint=getline(ss0, file, '/');
      mint=getline(ss0, more, '/');

      /* check for attr */
      if(reta && aint) {
        string attrs= database_getval(type,"attrs");
        stringstream ss3(attrs.c_str());
        reta=0;
        while(getline(ss3,token,':')){
          if(strcmp(attr.c_str(), token.c_str())==0){
            reta=1;
          }
        }

        /* check for val */
        if(reta && vint) {
          cout << val << endl;
          if(strcmp(attr.c_str(),("all_"+type+"s").c_str())==0) {
            clock_gettime(CLOCK_REALTIME,&stop);
            time_spent = (stop.tv_sec-start.tv_sec)+(stop.tv_nsec-start.tv_nsec)/BILLION; tot_time += time_spent;;
            access_avg_time=(access_avg_time*(access_calls-1)+time_spent)/access_calls;
            return 0;
          }
          string vals=database_getvals(attr);
          stringstream ss4(vals.c_str());
          reta=0;
          while(getline(ss4,token,':')){
            cout << val << token << endl;
            if(strcmp(val.c_str(), token.c_str())==0){
              reta=1;
            }
          }

          /* check for file */
          if(reta && fint) {
            cout << file << endl;
            string files=database_getval(attr, val);
            stringstream ss4(files.c_str());
            if(!mint) {
              reta=0;
              while(getline(ss4,token,':')){
                token=database_getval(token,"name");
                if(strcmp(file.c_str(), token.c_str())==0){
                  reta=1;
                }
              }
              stringstream ss5(attrs.c_str());
              while(getline(ss5,token,':')){
                if(strcmp(file.c_str(),token.c_str())==0){
                  reta=1;
                }
              }
            } else {
              found=1;
            }
          }
        }
      }
    }while(found);
  }

  if(reta && !getline(ss0, val, '/')) {
    return 0;
  }
  path=append_path(path);
  int ret = access(path, mask);
  return ret;
}


static int xmp_mknod(const char *path, mode_t mode, dev_t rdev) {
  log_msg("in xmp_mknod\n");

  path=append_path2(basename(strdup(path)));
  sprintf(msg,"khan_mknod, path=%s\n",path);
  log_msg(msg);
  int res;
  if (S_ISFIFO(mode))
    res = mkfifo(path, mode);
  else
    res = mknod(path, mode, rdev);
  if (res == -1) {
    fprintf(stderr, "\nmknod error \n");
    return -errno;
  }
  return 0;
}

static int xmp_mkdir(const char *path, mode_t mode) {
  struct timespec mkdir_start, mkdir_stop;
  sprintf(msg, "Khan mkdir: %s\n", path);
  log_msg(msg);
  string strpath=path;
  if(strpath.find("localize")!=string::npos) {
    if(strpath.find("usage")!=string::npos) {
      usage_localize();
    } else {
      /*cout << "LOCALIZING" << endl;
       *                  *cout << strpath << endl;
       *                                   *check location */
      string filename = "winter.mp3";
      string fileid = database_getval("name", filename);
      string location = get_location(fileid);
      string server = database_getval(fileid, "server");
      /*cout << "======== LOCATION: " << location << endl << endl;*/
      /*if not current*/
      if(location.compare(server)!=0) {
        /*  move to new location*/
        /*cout << " MUST MOVE "<<server<<" TO "<<location<<endl;*/
        database_setval(fileid,"server",location);
        string from = server + "/" + filename;
        string to = location + "/" + filename;
        string command = "mv " + from + " " + to;
        FILE* stream=popen(command.c_str(),"r");
        pclose(stream);
      }
    }
    /* cout << "LOCALIZATION TIME:" << localize_time << endl <<endl; */
    return -1;
  }
  if(strpath.find("stats")!=string::npos){
    /* print stats and reset */
    ofstream stfile;
    stfile.open(stats_file.c_str(), ofstream::out);
    stfile << "TOT TIME    :" << tot_time << endl;
    stfile << "Vold Calls   :" << vold_calls << endl;
    stfile << "     Avg Time:" << vold_avg_time << endl;
    stfile << "Readdir Calls:" << readdir_calls << endl;
    stfile << "     Avg Time:" << readdir_avg_time << endl;
    stfile << "Access Calls :" << access_calls << endl;
    stfile << "     Avg Time:" << access_avg_time << endl;
    stfile << "Read Calls   :" << read_calls << endl;
    stfile << "     Avg Time:" << read_avg_time << endl;
    stfile << "Getattr Calls:" << getattr_calls << endl;
    stfile << "     Avg Time:" << getattr_avg_time << endl;
    stfile << "Write Calls  :" << write_calls << endl;
    stfile << "     Avg Time:" << write_avg_time << endl;
    stfile << "Create Calls :" << create_calls << endl;
    stfile << "     Avg Time:" << create_avg_time << endl;
    stfile << "Rename Calls :" << rename_calls << endl;
    stfile << "     Avg Time:" << rename_avg_time << endl;
    stfile.close();
    vold_calls=0;
    readdir_calls=0;
    access_calls=0;
    getattr_calls=0;
    read_calls=0;
    write_calls=0;
    create_calls=0;
    rename_calls=0;
    tot_time=0;
    vold_avg_time=0;
    readdir_avg_time=0;
    access_avg_time=0;
    getattr_avg_time=0;
    read_avg_time=0;
    write_avg_time=0;
    create_avg_time=0;
    rename_avg_time=0;
    return -1;
  }

  log_msg("xmp_mkdir\n");
  sprintf(msg,"khan_mkdir for path=%s\n",path);
  log_msg(msg);
  struct stat *st;
  if(khan_getattr(path, st)<0) {
    /*add path*/
    database_setval("alldirs","paths",path);
    /*and break into attr/val pair and add to vold*/
  } else {
    log_msg("Directory exists\n");
  }
  return 0;
}


static int xmp_readlink(const char *path, char *buf, size_t size) {
  sprintf(msg, "Khan Read Link: %s\n", path);
  log_msg(msg);
  /* TODO: handle in vold somehow */
  log_msg("In readlink\n");
  int res = -1;
  path=append_path2(basename(strdup(path)));
  /* res = readlink(path, buf, size - 1); */
  if (res == -1)
    return -errno;
  buf[res] = '\0';
  return 0;
}

static int xmp_unlink(const char *path) {
  sprintf(msg, "Khan Unlink Directory: %s", path);
  log_msg(msg);
  /* TODO: handle in vold somehow */
  int res;
  string fileid=database_getval("name",basename(strdup(path)));

  string fromext=database_getval(fileid,"ext");
  string file=append_path2(basename(strdup(path)));
  string attrs=database_getval(fromext,"attrs");
  /*cout << fromext <<  fileid << endl;
   *          cout<<"HERE!"<<endl; */
  database_remove_val(fileid,"attrs","all_"+fromext+"s");
  /*cout<<"THERE!"<<endl;
   *database_remove_val("all_"+fromext+"s",strdup(basename(strdup(from))),fileid);
   *cout<<"WHERE!"<<endl;*/
  string token="";
  stringstream ss2(attrs.c_str());
  while(getline(ss2,token,':')){
    if(strcmp(token.c_str(),"null")!=0){
      string cmd=database_getval(token+"gen","command");
      string msg2=(cmd+" "+file).c_str();
      FILE* stream=popen(msg2.c_str(),"r");
      if(fgets(msg,200,stream)!=0){
        database_remove_val(fileid,token,msg);
      }
      pclose(stream);
    }
  }

  path=append_path2(basename(strdup(path)));
  res = unlink(path);
  if (res == -1)
    return -errno;
  return 0;
}


static int xmp_rmdir(const char *path) {
  sprintf(msg, "Khan Remove Directory: %s\n", path);
  log_msg(msg);
  /*if hardcoded, just remove*/
  database_remove_val("alldirs","paths",path);

  /*if exists
   *get contained files
   *get attrs+vals from path
   *unset files attrs
   *if entire attr, remove attr*/

  return 0;
}

static int xmp_symlink(const char *from, const char *to) {
  sprintf(msg, "Khan Sym Link Directory From: %s  To: %s", from, to);
  log_msg(msg);
  /*TODO: handle in vold somehow*/
  int res=-1;
  from=append_path2(basename(strdup(from)));
  to=append_path2(basename(strdup(to)));
  sprintf(msg,"In symlink creating a symbolic link from %s to %s\n",from, to);
  log_msg(msg);
  /*res = symlink(from, to);*/
  if (res == -1)
    return -errno;
  return 0;
}

static int xmp_link(const char *from, const char *to) {
  sprintf(msg, "Khan Link: From: %s To: %s\n", from , to);
  log_msg(msg);
  /*TODO:handle in vold somehow...*/
  int retstat = 0;
  from=append_path2(basename(strdup(from)));
  to=append_path2(basename(strdup(to)));
  sprintf(msg,"khan_link initial path=\"%s\", initial to=\"%s\")\n",from, to);
  log_msg(msg);
  retstat = link(from,to);
  return retstat;
}

static int xmp_chmod(const char *path, mode_t mode) {
  sprintf(msg, "Khan Chmod Directory: %s\n", path);
  log_msg(msg);

  int res;
  path=append_path2(basename(strdup(path)));
  sprintf(msg, "In chmod for: %s\n",path);
  log_msg(msg);
  res = chmod(path, mode);
#ifdef APPLE
  res = chmod(path, mode);
#else
  res = chmod(path, mode);
#endif
  if (res == -1)
    return -errno;
  return 0;
}

static int xmp_chown(const char *path, uid_t uid, gid_t gid) {
  sprintf(msg, "Khan Chown Directory: %s\n", path);
  log_msg(msg);
  int res;
  path=append_path2(basename(strdup(path)));
  sprintf(msg,"In chown for : %s\n",path);
  log_msg(msg);
  res = lchown(path,uid, gid);
  if (res == -1)
    return -errno;
  return 0;
}

static int xmp_truncate(const char *path, off_t size) {
  /*update for vold?*/
  sprintf(msg, "Khan Truncate Directory: %s", path);
  log_msg(msg);
  int res;
  path++;
  res = truncate(path, size);
  if (res == -1)
    return -errno;
  return 0;
}

static int xmp_utimens(const char *path, const struct timespec ts[2]) {
  log_msg("in utimens\n");
  int res;
  struct timeval tv[2];
  path=append_path2(basename(strdup(path)));
  sprintf(msg,"in utimens for path : %s\n",path);
  log_msg(msg);
  tv[0].tv_sec = ts[0].tv_sec;
  tv[0].tv_usec = ts[0].tv_nsec / 1000;
  tv[1].tv_sec = ts[1].tv_sec;
  tv[1].tv_usec = ts[1].tv_nsec / 1000;
  res = utimes(path, tv);
  if (res == -1)
    return -errno;
  return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
  int res = 0;
  sprintf(msg, "Khan xmp_read: %s", path);
  log_msg(msg);
  path=append_path2(basename(strdup(path)));
  /* cout<<"Converted Path: "<<path<<endl<<endl<<endl<<endl; */

  FILE *thefile = fopen(path, "r");
  if (thefile != NULL){
    fseek(thefile, offset, SEEK_SET);
    res = fread(buf, 1, size, thefile);
    /* cout << "READ THIS MANY"<<endl<<res<<endl<<endl<<endl<<endl<<endl; */

    if (res == -1)
      res = -errno;
    fclose(thefile);
  } else {
    res = -errno;
  }
  return res;
}

static int xmp_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
  sprintf(msg, "Khan xmp_write: %s\n", path);
  log_msg(msg);
  int fd;
  int res;

  path=append_path2(basename(strdup(path)));
  (void) fi;
  fd = open(path, O_WRONLY);
  if (fd == -1){
    return errno;
  }
  res = pwrite(fd, buf, size, offset);
  if (res == -1)
    res = errno;
  close(fd);
  return res;
}

static int xmp_statfs(const char *path, struct statvfs *stbuf) {
  /* Pass the call through to the underlying system which has the media. */
  sprintf(msg, "Khan xmp_statfs: %s\n", path);
  log_msg(msg);
  int res = statvfs(path, stbuf);
  if (res != 0) {
    fprintf(log, "statfs error for %s\n",path);
    return errno;
  }
  return 0;
}

static int xmp_release(const char *path, struct fuse_file_info *fi) {
  /* Just a stub. This method is optional and can safely be left unimplemented. */
  fprintf(log, "in xmp_release with path %s\n", path);
  return 0;
}

static int xmp_fsync(const char *path, int isdatasync,struct fuse_file_info *fi) {
  /* Just a stub. This method is optional and can safely be left unimplemented. */
  fprintf(log, "in xmp_fsync with path %s\n", path);
  return 0;
}

static int xmp_rename(const char *from, const char *to) {
  sprintf(msg, "Khan Rename Directory From: %s To: %s", from , to);
  log_msg(msg);
  /* cout << endl << endl << endl << "Entering Rename Function" << endl; */
  double start_time = 0;
  struct timeval start_tv;
  gettimeofday(&start_tv, NULL); 
  start_time = start_tv.tv_sec;
  start_time += (start_tv.tv_usec/1000000.0);
  start_times << fixed << start_time << endl << flush;
  string src = basename(strdup(from));
  string dst = basename(strdup(to));
  string fileid = database_getval("name", src);
  /* cout << fileid << endl; */
  database_remove_val(fileid,"name",src);
  /* cout << src << endl; */
  database_setval(fileid,"name",dst);
  /* cout << dst << endl; */
  string orig_path = append_path2(src);
  string orig_loc = database_getval(fileid,"location");
  map_path(resolve_selectors(to), fileid);
  string new_path = append_path2(dst);
  string new_loc = database_getval(fileid,"location");
  if(new_loc!=orig_loc) {
    if(new_loc=="google_music") {
      /* upload */
      /* cloud_upload(orig_path); */
    } else if(orig_loc=="google_music") {
      /* download */
      /* cloud_download(src, new_path); */
    } else {
      /* file system rename */
      rename(orig_path.c_str(), new_path.c_str());
    }
  }
  double rename_time = 0;
  struct timeval end_tv;
  gettimeofday(&end_tv, NULL); 
  rename_time = end_tv.tv_sec - start_tv.tv_sec;
  rename_time += (end_tv.tv_usec - start_tv.tv_usec) / 1000000.0;
  rename_times << fixed << rename_time << endl << flush;
  /* cout << "Exiting Rename Function" << endl << endl << endl << endl; */
  return 0;
}

#ifdef APPLE
static int xmp_setxattr(const char *path, const char *name, const char *value,  size_t size, int flags, uint32_t param) {
#else
  static int xmp_setxattr(const char *path, const char *name, const char *value,  size_t size, int flags) {
#endif
    string attr = bin2hex(value, size);
    sprintf(msg, "setxattr call\npath:%sname:%svalue:%s\n\n\n\n\n\n\n\n\n\n", path, name, value);
    log_msg(msg);
    string xpath = "xattr:";
    xpath += path;
    redis_setval(xpath, name, attr.c_str());
    sprintf(msg, "setxattr call\n %s, %s, %s\n\n\n\n\n\n\n\n\n\n", xpath.c_str(), name, attr.c_str());
    log_msg(msg);
    return 0;
  }
#ifdef APPLE
  static int xmp_getxattr(const char *path, const char *name, char *value, size_t size, uint32_t param) {
#else
    static int xmp_getxattr(const char *path, const char *name, char *value, size_t size) {
#endif
      fprintf(stderr, "getxattr call\n %s, %s, %s\n", path, name, value);
      string xpath = "xattr:";
      xpath += path;
      string db_val = redis_getval( xpath, name);
      sprintf(msg, "getxattr call\npath:%s\nname:%s\nvalue:%s\nsize:%zd\n\n\n\n\n", xpath.c_str(), name, db_val.c_str(), size);
      log_msg(msg);
      if(db_val != "null") {
        db_val = hex2bin(db_val);
        if(value==NULL) {
          errno = 0;
          return db_val.length();
        }
        memcpy(value, db_val.c_str(), size);
        size_t num = snprintf(value, size, "%s", db_val.c_str());
        sprintf(msg, "returned\nstring:%s\ncount:%zd\n\n", value, num);
        log_msg(msg);
        errno = 0;
        return size;
      }
      errno = 1;
      return -1;
    }

    static int xmp_listxattr(const char *path, char *list, size_t size) {
      sprintf(msg, "listxattr call\n %s, %s\n\n", path, list);
      log_msg(msg);
      string xpath = "xattr:";
      xpath += path;
      string attrs = database_getvals(xpath);
      char* mal = strdup(attrs.c_str());
      int count = 1;
      int str_size = strlen(mal);
      for(int i = 0; i<str_size; i++) {
        if(mal[i]==':') {
          mal[i]='\0';
          count += 1;
        }
      }
      if(list==NULL) {
        sprintf(msg, "returning %d\n", str_size);
        log_msg(msg);
        return str_size;
      }
      snprintf(list, size, "%s", attrs.c_str());
      sprintf(msg, "returning %s and %d\n", list, count);
      log_msg(msg);
      return count;
    }

    static int xmp_removexattr(const char *path, const char *name) {
      fprintf(stderr, "removexattr call\n %s, %s\n\n", path, name);
      return 0;
    }

#ifdef APPLE

    static int xmp_setvolname(const char* param) {
      fprintf(stderr, "apple function called setvolname\n");
      return 0;
    }

    static int xmp_exchange(const char* param1, const char* param2, unsigned long param3) {
      fprintf(stderr, "apple function called exchange\n");
      return 0;
    }

    static int xmp_getxtimes(const char* param1, struct timespec* param2, struct timespec* param3) {
      fprintf(stderr, "apple function called xtimes\n");
      return 0;
    }

    static int xmp_setbkuptime(const char* param1, const struct timespec* param2) {
      fprintf(stderr, "apple function called setbkuptimes\n");
      return 0;
    }

    static int xmp_setchgtime(const char* param1, const struct timespec* param2) {
      fprintf(stderr, "apple function called setchgtimes\n");
      return 0;
    }

    static int xmp_setcrtime(const char* param1, const struct timespec* param2) {
      fprintf(stderr, "apple function called setcrtimes\n");
      return 0;
    }

    static int xmp_chflags(const char* param1, uint32_t param2) {
      fprintf(stderr, "apple function called chflags\n");
      return 0;
    }
    static int xmp_setattr_x(const char* param1, struct setattr_x* param2) {
      fprintf(stderr, "apple function called setattr_x\n");
      return 0;
    }

    static int xmp_fsetattr_x(const char* param1, struct setattr_x* param2, struct fuse_file_info* param3) {
      fprintf(stderr, "apple function called fsetattr_x\n");
      return 0;
    }

