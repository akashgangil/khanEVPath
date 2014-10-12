
#ifndef FUSE_API_H
#define FUSE_API_H

#include "fuse.h"

void xmp_initialize(void);

void *khan_init(struct fuse_conn_info*);

int khan_flush(const char*, struct fuse_file_info*); 

int khan_create(const char *, mode_t, struct fuse_file_info*);

int khan_open(const char*, struct fuse_file_info*);

int xmp_access(const char *path, int mask);

int xmp_mknod(const char *path, mode_t mode, dev_t rdev);

int xmp_mkdir(const char *path, mode_t mode);
        
int xmp_readlink(const char *path, char *buf, size_t size);
   
int xmp_unlink(const char *path); 
   
int xmp_rmdir(const char *path); 
    
int xmp_symlink(const char *from, const char *to); 
   
int xmp_link(const char *from, const char *to); 
        
int xmp_chmod(const char *path, mode_t mode); 
        
int xmp_chown(const char *path, uid_t uid, gid_t gid); 

int xmp_truncate(const char *path, off_t size); 
    
int xmp_utimens(const char *path, const struct timespec ts[2]); 
    
int xmp_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi); 
    
int xmp_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi); 
    
int xmp_statfs(const char *path, struct statvfs *stbuf); 
    
int xmp_release(const char *path, struct fuse_file_info *fi); 

int xmp_fsync(const char *path, int isdatasync,struct fuse_file_info *fi); 

int xmp_rename(const char*, const char*);

int xmp_readdir(const char*, void*, fuse_fill_dir_t, off_t, struct fuse_file_info*);

int xmp_getxattr(const char*, const char*, char*, size_t);

#endif
