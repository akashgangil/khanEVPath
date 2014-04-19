
#ifndef FUSE_H
#define FUSE_H

static void xmp_initialize(void);

static int xmp_access(const char *path, int mask);

static int xmp_mknod(const char *path, mode_t mode, dev_t rdev);
    
static int xmp_mkdir(const char *path, mode_t mode);
        
static int xmp_readlink(const char *path, char *buf, size_t size);
   
static int xmp_unlink(const char *path); 
   
static int xmp_rmdir(const char *path); 
    
static int xmp_symlink(const char *from, const char *to); 
   
static int xmp_link(const char *from, const char *to); 
        
static int xmp_chmod(const char *path, mode_t mode); 
        
static int xmp_chown(const char *path, uid_t uid, gid_t gid); 

static int xmp_truncate(const char *path, off_t size); 
    
static int xmp_utimens(const char *path, const struct timespec ts[2]); 
    
static int xmp_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi); 
    
static int xmp_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi); 
    
static int xmp_statfs(const char *path, struct statvfs *stbuf); 
    
static int xmp_release(const char *path, struct fuse_file_info *fi); 
    
static int xmp_fsync(const char *path, int isdatasync,struct fuse_file_info *fi); 

#endif FUSE_H
