void* initializing_khan(void * mnt_dir) {
    log_msg("In initialize\n");
    /* unmounting((char *)mnt_dir); */
    /* Opening root directory and creating if not present */
    sprintf(msg, "khan_root[0] is %s\n", servers.at(0).c_str());
    log_msg(msg);
    /* cout<<"khan_root[0] is "<<servers.at(0)<<endl; */
    if(NULL == opendir(servers.at(0).c_str()))  {
        sprintf(msg,"Error msg on opening directory : %s\n",strerror(errno));
        log_msg(msg);
        log_msg("Root directory might not exist..Creating\n");
        string command = "mkdir " + servers.at(0);
        if (system(command.c_str()) < 0) {
            log_msg("Unable to create storage directory...Aborting\n");
            exit(1);
        }
    } else {
        log_msg("directory opened successfully\n");
    }

    init_database();

    /* check if we've loaded metadata before */
    string output=database_getval("setup","value");
    if(output.compare("true")==0){
        log_msg("Database was previously initialized.");
        tot_time+=(stop.tv_sec-start.tv_sec)+(stop.tv_nsec-start.tv_nsec)/BILLION;
        return 0; /* setup has happened before */
    }

    /* if we have not setup, do so now */
    log_msg("it hasnt happened, setvalue then setup\n");
    database_setval("setup","value","true");

    /* load metadata associatons */
    for(int i=0; i<servers.size(); i++){
        /* log_msg("servers: " + servers.at(i) + "\n"); */
        process_transducers(servers.at(i));
    }

    /* load metadata for each file on each server */
    string types=database_getval("allfiles","types");
    sprintf(msg, "=== types to look for = %s\n", types.c_str());
    log_msg(msg);   

    /* log_msg("Server Size" + servers.size() + "\n"); */
    for(int i=0; i<servers.size(); i++) {
        /*    if(servers.at(i) == "cloud") {
         *      cout << " Cloud \n";
         *      PyObject* myFunction = PyObject_GetAttrString(cloud_interface,(char*)"get_all_titles");
         *      PyObject* myResult = PyObject_CallObject(myFunction, NULL);
         *      if(myResult==NULL) {
         *         PyErr_PrintEx(0);
         *         continue;
         *      }
         *      int n = PyList_Size(myResult);
         *      cout << "SIZE = " << n << endl << flush; 
         */
        for(int j = 0; j<n; j++) {
            PyObject* title = PyList_GetItem(myResult, j);
            char* temp = PyString_AsString(title);
            if(temp==NULL) {
                PyErr_PrintEx(0);
                continue;
            }
            string filename = temp;
            /* cout << "Checking " << filename << " ... " << endl << flush; */
            if(database_getval("name",filename)=="null") {
                string fileid = database_setval("null","name",filename);
                string ext = strrchr(filename.c_str(),'.')+1;
                database_setval(fileid,"ext",ext);
                database_setval(fileid,"server",servers.at(i));
                database_setval(fileid,"location",server_ids.at(i));
                string attrs=database_getval(ext,"attrs");
                string token="";
                stringstream ss2(attrs.c_str());
                PyObject* myFunction = PyObject_GetAttrString(cloud_interface,(char*)"get_metadata");
                while(getline(ss2,token,':')){
                    if(strcmp(token.c_str(),"null")!=0){

                        PyObject* arglist = PyTuple_New(2);
                        PyTuple_SetItem(arglist, 0, PyString_FromString(filename.c_str()));
                        PyTuple_SetItem(arglist, 1, PyString_FromString(token.c_str()));
                        PyObject* myResult = PyObject_CallObject(myFunction, arglist);

                        if(myResult==NULL) {
                            PyErr_PrintEx(0);
                            continue;
                        }
                        char* msg = PyString_AsString(myResult);
                        if(!msg) {
                            PyErr_PrintEx(0);
                            continue;
                        }
                        string val = msg;
                        Py_DECREF(arglist);
                        Py_DECREF(myResult);

                        if(val!="na") {
                            database_setval(fileid,token,val);
                        }
                    }
                }
            }
        }
    } else {
            log_msg("Not Cloud \n");

        /*      string command = "find -type d | awk -F'/' '{print NF-1}' | sort -n | tail -1"; */
        /* string command = "find /net/hp100/ihpcae -type d | awk -F'/' '{print NF-1}' | sort -n | tail -1"; */

            glob_t files;
        string pattern= servers.at(0) + "/*";
        static int experiment_id = 0;
        set<string> experiments;

        for(int count = 18; count > 0; count--)
        {  
            sprintf(msg, "Globbing with pattern: %s .im7\n", pattern.c_str());
            /* log_msg("Globbing with pattern " + pattern + ".im7\n"); */
            log_msg(msg); 
            glob((pattern +".im7").c_str(), 0, NULL, &files);

            sprintf(msg, "Glob Buffer: %d\n", files.gl_pathc); 
            log_msg(msg);

            /*log_msg("Glob buffer" + files.gl_pathc + "\n");
             *                      if(files.gl_pathc != 0 ) {
             *                                               experiment_id++;
             *                                                                   } */

            for(int j=0; j<files.gl_pathc; j++) {/* for each file */
                string file_path = files.gl_pathv[j];
                /* experiments.insert(file_path.substr(0, file_path.size()-11));
                 *                         ostringstream ss;
                 *                                                 ss.flush();
                 *                                                                         ss << experiments.size(); */
                sprintf(msg, "*** FILE Path *** %s\n", file_path.c_str());
                string ext = strrchr(file_path.c_str(),'.')+1;
                string filename=strrchr(file_path.c_str(),'/')+1;
                if(database_getval("name", filename) == "null" || 1) {
                    string fileid = database_setval("null","name",filename);
                    database_setval(fileid,"ext",ext);
                    database_setval(fileid,"server",servers.at(i));
                    database_setval(fileid,"location",server_ids.at(i));
                    /*                          database_setval(fileid, "experiment_id", ss.str()); */
                    database_setval(fileid, "file_path", file_path);
                    for(int k=0; k<server_ids.size(); k++) {
                        database_setval(fileid, server_ids.at(k), "0");
                    }
                    process_file(servers.at(i), fileid, file_path);
                } else {
                    string fileid = database_getval("name",filename);
                    database_setval(fileid,"server",servers.at(i));
                    database_setval(fileid,"location",server_ids.at(i));
                }
            }
            pattern += "/*";
        }


        log_msg("At the end of initialize\n");
        analytics();
        return 0;
    }
    }


    int khan_opendir(const char *c_path, struct fuse_file_info *fi) {
        return 0;
    }

    bool find(string str, vector<string> arr) {
        for(int i=0; i<arr.size(); i++) {
            if(str == arr[i]) return true;
        }
        return false;
    }

    string str_intersect(string str1, string str2) {
        vector<string> vec_1 = split(str1, ":");
        vector<string> vec_2 = split(str2, ":");
        vector<string> ret;
        for(int i=0; i<vec_1.size(); i++) {
            for(int j=0; j<vec_2.size(); j++) {
                if((vec_1[i]==vec_2[j]) && (!find(vec_1[i], ret))) {
                    ret.push_back(vec_1[i]);
                }
            }
        }
        return join(ret,":");
    }

    bool content_has(string vals, string val) {
        static string last_string = "";
        static vector<string> last_vector;
        vector<string> checks;

        if(last_string == vals){
            checks = last_vector;
        } else {
            checks = split(vals,":");
            last_string = vals;
            last_vector = checks;
        }

        for(int i=0; i<checks.size(); i++) {
            if(checks[i]==val) {
                return true;
            }
        }
        return false;
    }

    void dir_pop_stbuf(struct stat* stbuf, string contents) {
        cout << "Dir pop STBUF called ! " << endl;
        time_t current_time;
        time(&current_time);
        stbuf->st_mode=S_IFDIR | 0755;
        stbuf->st_nlink=count_string(contents)+2;
        stbuf->st_size=4096;
        stbuf->st_mtime=current_time;
        stbuf->st_uid = getuid();
        stbuf->st_gid = getgid();
    }

    void file_pop_stbuf(struct stat* stbuf, string filename) {
        time_t current_time;
        time(&current_time);
        stbuf->st_mode=S_IFREG | 0644;
        stbuf->st_nlink=1;
        stbuf->st_size=get_file_size(filename);
        stbuf->st_mtime=current_time;
        stbuf->st_uid = getuid();
        stbuf->st_gid = getgid();
    }

    string resolve_selectors(string path) {
        /* cout << "starting split" << endl << flush; */
        vector<string> pieces = split(path, "/");
        /* cout << "starting process" << endl << flush; */
        for(int i=0; i<pieces.size(); i++) {
            /* cout << "looking at " << pieces[i] << endl << flush; */
            if(pieces[i].at(0)==SELECTOR_C) {
                /* cout << "is a selector" << endl << flush; */
                vector<string> selectores = split(pieces[i], SELECTOR_S);
                pieces[i]="";
                /* cout << selectores.size() << " selectors to be exact" << endl << flush; */
                for(int j=0; j<selectores.size(); j++) {
                    /* cout << "checking " << selectores[j] << endl << flush; */
                    bool matched = false;
                    string content = database_getvals("attrs");
                    /* cout << "content " << content << endl << flush; */
                    vector<string> attr_vec = split(content, ":");
                    /* cout << "vs " << attr_vec.size() << " attrs" << endl << flush; */
                    /* for all attrs */
                    for(int k=0; k<attr_vec.size(); k++) {
                        /* cout << "on " << attr_vec[k] << endl << flush; */
                        string vals = database_getvals(attr_vec[k]);
                        /* cout << "with " << vals << endl << flush; */
                        /* see if piece is in vals */
                        if(content_has(vals, selectores[j])) {
                            /* if so piece now equals attr/val */
                            if(pieces[i].length()>0) {
                                pieces[i]+="/";
                            }
                            matched = true;
                            pieces[i]+=attr_vec[k]+"/"+selectores[j];
                            break;
                        }
                    }
                    if(!matched) {
                        pieces[i]+="tags/"+selectores[j];
                    }
                }
            }
        }
        string ret = join(pieces, "/");
        /* cout << "selector path " << path << " resolved to " << ret << endl; */
        return ret;
    }

    int populate_getattr_buffer(struct stat* stbuf, stringstream &path) {
        string attr, val, file, more;
        string current = "none";
        string current_path =  path.str();
        void* aint=getline(path, attr, '/');
        void* vint=getline(path, val, '/');
        void* fint=getline(path, file, '/');
        void* mint=getline(path, more, '/');
        bool loop = true;
        while(loop) {
            /* cout << "top of loop" << endl << flush; */
            loop = false;
            if(aint) {
                string query = database_getval("attrs", attr);
                /*    cout << "PrINT "  << "  " << attr << " " << query << endl; */
                if(query!="null") {
                    string content = database_getvals(attr);
                    /*      cout << "Query not null   " << content << endl; */
                    if(vint) {
                        /*          cout << "Vint is true " << endl; */
                        cout << "Value is " << content_has(content, val) << endl;
                        cout << "Val is  " << val << endl; */
                            if(content_has(content, val) || (attr=="tags")) {
                                /*        cout << "Here1 " << val << endl; */
                                string dir_content = database_getval(attr, val);
                                if(current!="none") {
                                    dir_content = str_intersect(current, dir_content);
                                }
                                string attrs_content = database_getvals("attrs");
                                if(fint) {
                                    /*          cout << "fint is true " << file << endl; */
                                    string fileid = database_getval("name",file);
                                    if(content_has(dir_content, fileid)) {
                                        if(!mint) {
                                            /* /attr/val/file path */
                                            string file_path = database_getval(fileid, "file_path");
                                            file_pop_stbuf(stbuf, file_path);
                                            return 0;
                                        }
                                    } else if(content_has(attrs_content, file)) {
                                        /* repeat with aint = fint, vint = mint, etc */
                                        aint = fint;
                                        attr = file;
                                        vint = mint;
                                        val = more;
                                        fint=getline(path, file, '/');
                                        mint=getline(path, more, '/');
                                        current = dir_content;
                                        loop = true;
                                    }
                                } else {
                                    /* /attr/val dir */
                                    cout << "Fint is flase " << dir_content + attrs_content << endl;
                                    /* if(dir_content != ""){ */
                                    dir_pop_stbuf(stbuf, dir_content+attrs_content);
                                    return 0;
                                }  
                            } 
                    } else { 
                        /* /attr dir */
                        /* cout << " Here 2  " << content << endl; */
                        dir_pop_stbuf(stbuf, content);
                        return 0;
                    }
                }
            } else {
                string types=database_getvals("attrs");
                /* cout << " HEre 3 " << types << endl; */
                dir_pop_stbuf(stbuf, types);
                return 0;
            }
        }
        return -2;
    }

    static int khan_getattr(const char *c_path, struct stat *stbuf) {
        /* cout << "started get attr" << endl << flush; */
        string pre_processed = c_path+1;
        if(pre_processed == ".DS_Store") {
            file_pop_stbuf(stbuf, pre_processed);
            return 0;
        }
        /* cout << "starting to resolve selectors" << endl << flush; */
        string after = resolve_selectors(pre_processed);
        stringstream path(after);
        /*cout << "working to pop buffer" << endl << flush;
         *         file_pop_stbuf(stbuf, "test");
         *                 int ret = 0; */
        int ret = populate_getattr_buffer(stbuf, path);
        /* cout << "ended get attr" << endl << flush; */
        return ret;
    }

    void dir_pop_buf(void* buf, fuse_fill_dir_t filler, string content, bool convert) {

        sprintf(msg, "Inside dir_pop_buf: %s\n", content.c_str());
        log_msg(msg);

        vector<string> contents = split(content, ":");
        for(int i=0; i<contents.size(); i++) {
            if(convert) {
                string filename = database_getval(contents[i].c_str(), "name");

                sprintf(msg, "dir_pop_buf loop%s\n", filename.c_str());
                log_msg(msg);

                filler(buf, filename.c_str(), NULL, 0);
            } else {
                cout << "Convert is false " << endl;
                filler(buf, contents[i].c_str(), NULL, 0);
            }
        }
    }


    void populate_readdir_buffer(void* buf, fuse_fill_dir_t filler, stringstream &path) {

        log_msg("Populate Read Dir Buffer\n");
        sprintf(msg, "Path is %s\n", path.str().c_str());
        log_msg(msg);
        string attr, val, file, more;
        string current_content = "none";
        string current_attrs = "none";
        string non_empty_content = "";
        void* aint=getline(path, attr, '/');
        void* vint=getline(path, val, '/');
        void* fint=getline(path, file, '/');
        void* mint=getline(path, more, '/');

        bool loop = true;
        while(loop) {
            loop = false;
            cout << "HO HO JUMPING!!  " << endl; 
            string content = database_getvals("attrs");

            cout << "Attrs is " << content << endl;

            if(aint) {
                cout << "Aint is true " << endl;
                cout << "Attr " << attr << endl;
                if(content_has(content, attr)) {
                    current_attrs += ":";
                    current_attrs += attr; 
                    content = database_getvals(attr);

                    cout << "Current Content   " << current_content << endl;                


                    if(current_content != "none"){
                        cout << "Content " << content << endl;

                        non_empty_content = "";

                        vector<string> vec_1 = split(content, ":");
                        for(int i = 0; i < vec_1.size(); ++i){
                            cout << "Attr " << attr << "  Val " << vec_1[i] << endl;
                            string dir_content = database_getval(attr, vec_1[i]);
                            if(current_content!="none") {
                                dir_content = str_intersect(current_content, dir_content);
                            }
                            cout << "Iteration " << " ** " << i << "   " << dir_content << endl;
                            if(dir_content != ""){
                                non_empty_content += vec_1[i] + ":";
                            }
                        }
                    }else non_empty_content = content;


                    cout << "Non Empty Content  " << non_empty_content << endl;

                    if(vint) {
                        cout << " Content is " << content << endl;
                        cout << "Value is  " << content_has(content, val) << endl;
                        cout << "Vint is true " << endl;
                        if(content_has(content, val) || (attr=="tags")) {
                            string dir_content = database_getval(attr, val);
                            cout << " ABRA  " << endl;
                            if(current_content!="none") {
                                cout << " f sdfdsfsdf " << endl;
                                dir_content = intersect(current_content, dir_content);
                            }
                            string attrs_content = database_getvals("attrs");
                            if(fint) {
                                cout << "Fint is true " << endl;
                                if(content_has(attrs_content, file)) {
                                    /* repeat with aint = fint, vint = mint, etc */
                                    aint = fint;
                                    attr = file;
                                    vint = mint;
                                    val = more;
                                    fint = getline(path, file, '/');
                                    mint = getline(path, more, '/');
                                    current_content = dir_content;
                                    loop = true;
                                }
                            } else {
                                /* /attr/val dir */
                                sprintf(msg, "Else %s, %s\n\n\n\n\n\n", attrs_content.c_str(), current_attrs.c_str());
                                log_msg(msg);
                                attrs_content = subtract(attrs_content, current_attrs);
                                cout << "Dir Content  " << dir_content << endl;
                                cout << "Attr Content  " << attrs_content << endl;
                                dir_pop_buf(buf, filler, dir_content, true);
                                dir_pop_buf(buf, filler, attrs_content, false);
                            }  
                        } 
                    } else { 
                        /* /attr dir */
                        cout << "Going solo1 " << non_empty_content << endl;
                        dir_pop_buf(buf, filler, non_empty_content, false);
                    }
                }
            } else {
                cout << " Going solo2 " << endl;
                dir_pop_buf(buf, filler, content, false);
            }
        }
        cout << "populate read dir end  " << endl;
    }
