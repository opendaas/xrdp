/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall `pkg-config fuse --cflags --libs` fusexmp.c -o fusexmp
*/

#define FUSE_USE_VERSION 26



#ifdef linux
/* For pread()/pwrite() */
#define _XOPEN_SOURCE 500
#endif


#include "disk_dev.h"


static struct disk_device disk_devices[128];
static int disk_devices_count = 0;
extern struct log_config *l_config;
extern char mount_point[256];


/*****************************************************************************/
int APP_CC
disk_dev_in_unistr(struct stream* s, char *string, int str_size, int in_len)
{
#ifdef HAVE_ICONV
  size_t ibl = in_len, obl = str_size - 1;
  char *pin = (char *) s->p, *pout = string;
  static iconv_t iconv_h = (iconv_t) - 1;

  if (g_iconv_works)
  {
    if (iconv_h == (iconv_t) - 1)
    {
      if ((iconv_h = iconv_open(g_codepage, WINDOWS_CODEPAGE)) == (iconv_t) - 1)
      {
        log_message(l_config, LOG_LEVEL_WARNING, "rdpdr_disk[disk_dev_in_unistr]: "
        		"Iconv_open[%s -> %s] fail %p",
					WINDOWS_CODEPAGE, g_codepage, iconv_h);
        g_iconv_works = False;
        return rdp_in_unistr(s, string, str_size, in_len);
      }
    }

    if (iconv(iconv_h, (ICONV_CONST char **) &pin, &ibl, &pout, &obl) == (size_t) - 1)
    {
      if (errno == E2BIG)
      {
        log_message(l_config, LOG_LEVEL_WARNING, "rdpdr_disk[disk_dev_in_unistr]: "
							"Server sent an unexpectedly long string, truncating");
      }
      else
      {
        iconv_close(iconv_h);
        iconv_h = (iconv_t) - 1;
        log_message(l_config, LOG_LEVEL_WARNING, "rdpdr_disk[disk_dev_in_unistr]: "
							"Iconv fail, errno %d\n", errno);
        g_iconv_works = False;
        return rdpdr_in_unistr(s, string, str_size, in_len);
      }
    }

    /* we must update the location of the current STREAM for future reads of s->p */
    s->p += in_len;
    *pout = 0;
    return pout - string;
  }
  else
#endif
  {
    int i = 0;
    int len = in_len / 2;
    int rem = 0;

    if (len > str_size - 1)
    {
      log_message(l_config, LOG_LEVEL_WARNING, "rdpdr_disk[disk_dev_in_unistr]: "
						"server sent an unexpectedly long string, truncating");
      len = str_size - 1;
      rem = in_len - 2 * len;
    }
    while (i < len)
    {
      in_uint8a(s, &string[i++], 1);
      in_uint8s(s, 1);
    }
    in_uint8s(s, rem);
    string[len] = 0;
    return len;
  }
}

/************************************************************************/
int APP_CC
disk_dev_get_dir(int device_id)
{
	int i;
	for (i=0 ; i< disk_devices_count ; i++)
	{
		if(device_id == disk_devices[i].device_id)
		{
			return i;
		}
	}
	return -1;
}

/************************************************************************/
int APP_CC
disk_dev_add(struct stream* s, int device_data_length,
								int device_id, char* dos_name)
{


	disk_devices[disk_devices_count].device_id = device_id;
	g_strcpy(disk_devices[disk_devices_count].dir_name, dos_name);
	disk_devices_count++;
	log_message(l_config, LOG_LEVEL_DEBUG, "rdpdr_disk[disk_dev_add]: "
				"Succedd to add disk");
  return g_time1();
}


/********************************************************************************************
	Begin of fuse operation
 */

static int disk_dev_getattr(const char *path, struct stat *stbuf)
{
	log_message(l_config, LOG_LEVEL_DEBUG, "disk_dev_getattr\n");
	int res;

	res = lstat(path, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int disk_dev_access(const char *path, int mask)
{
	log_message(l_config, LOG_LEVEL_DEBUG, "disk_dev_access\n");
	int res;

	res = access(path, mask);
	if (res == -1)
		return -errno;

	return 0;
}

static int disk_dev_readlink(const char *path, char *buf, size_t size)
{
	log_message(l_config, LOG_LEVEL_DEBUG, "disk_dev_readlink\n");
	int res;

	res = readlink(path, buf, size - 1);
	if (res == -1)
		return -errno;

	buf[res] = '\0';
	return 0;
}


static int disk_dev_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
	DIR *dp;
	struct dirent *de;
	log_message(l_config, LOG_LEVEL_DEBUG, "disk_dev_readdir\n");

	(void) offset;
	(void) fi;

	dp = opendir(path);
	if (dp == NULL)
		return -errno;

	while ((de = readdir(dp)) != NULL) {
		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = de->d_ino;
		st.st_mode = de->d_type << 12;
		if (filler(buf, de->d_name, &st, 0))
			break;
	}

	closedir(dp);
	return 0;
}

static int disk_dev_mknod(const char *path, mode_t mode, dev_t rdev)
{
	log_message(l_config, LOG_LEVEL_DEBUG, "disk_dev_mknod\n");
	int res;

	/* On Linux this could just be 'mknod(path, mode, rdev)' but this
	   is more portable */
	if (S_ISREG(mode)) {
		res = open(path, O_CREAT | O_EXCL | O_WRONLY, mode);
		if (res >= 0)
			res = close(res);
	} else if (S_ISFIFO(mode))
		res = mkfifo(path, mode);
	else
		res = mknod(path, mode, rdev);
	if (res == -1)
		return -errno;

	return 0;
}

static int disk_dev_mkdir(const char *path, mode_t mode)
{
	log_message(l_config, LOG_LEVEL_DEBUG, "disk_dev_mkdir\n");
	int res;

	res = mkdir(path, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int disk_dev_unlink(const char *path)
{
	log_message(l_config, LOG_LEVEL_DEBUG, "disk_dev_unlink\n");
	int res;

	res = unlink(path);
	if (res == -1)
		return -errno;

	return 0;
}

static int disk_dev_rmdir(const char *path)
{
	log_message(l_config, LOG_LEVEL_DEBUG, "disk_dev_rmdir\n");
	int res;

	res = rmdir(path);
	if (res == -1)
		return -errno;

	return 0;
}

static int disk_dev_symlink(const char *from, const char *to)
{
	log_message(l_config, LOG_LEVEL_DEBUG, "disk_dev_symlink\n");
	int res;

	res = symlink(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int disk_dev_rename(const char *from, const char *to)
{
	log_message(l_config, LOG_LEVEL_DEBUG, "disk_dev_rename\n");
	int res;

	res = rename(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int disk_dev_link(const char *from, const char *to)
{
	log_message(l_config, LOG_LEVEL_DEBUG, "disk_dev_link\n");
	int res;

	res = link(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int disk_dev_chmod(const char *path, mode_t mode)
{
	log_message(l_config, LOG_LEVEL_DEBUG, "disk_dev_chmod\n");
	int res;

	res = chmod(path, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int disk_dev_chown(const char *path, uid_t uid, gid_t gid)
{
	log_message(l_config, LOG_LEVEL_DEBUG, "disk_dev_chown\n");
	int res;

	res = lchown(path, uid, gid);
	if (res == -1)
		return -errno;

	return 0;
}

static int disk_dev_truncate(const char *path, off_t size)
{
	log_message(l_config, LOG_LEVEL_DEBUG, "disk_dev_truncate\n");
	int res;

	res = truncate(path, size);
	if (res == -1)
		return -errno;

	return 0;
}

static int disk_dev_utimens(const char *path, const struct timespec ts[2])
{
	log_message(l_config, LOG_LEVEL_DEBUG, "disk_dev_utimens\n");
	int res;
	struct timeval tv[2];

	tv[0].tv_sec = ts[0].tv_sec;
	tv[0].tv_usec = ts[0].tv_nsec / 1000;
	tv[1].tv_sec = ts[1].tv_sec;
	tv[1].tv_usec = ts[1].tv_nsec / 1000;

	res = utimes(path, tv);
	if (res == -1)
		return -errno;

	return 0;
}

static int disk_dev_open(const char *path, struct fuse_file_info *fi)
{
	log_message(l_config, LOG_LEVEL_DEBUG, "disk_dev_open\n");
	int res;

	res = open(path, fi->flags);
	if (res == -1)
		return -errno;

	close(res);
	return 0;
}

static int disk_dev_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	log_message(l_config, LOG_LEVEL_DEBUG, "disk_dev_read\n");
	int fd;
	int res;

	(void) fi;
	fd = open(path, O_RDONLY);
	if (fd == -1)
		return -errno;

	res = pread(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	close(fd);
	return res;
}

static int disk_dev_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	log_message(l_config, LOG_LEVEL_DEBUG, "disk_dev_write\n");
	int fd;
	int res;

	(void) fi;
	fd = open(path, O_WRONLY);
	if (fd == -1)
		return -errno;

	res = pwrite(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	close(fd);
	return res;
}

static int disk_dev_statfs(const char *path, struct statvfs *stbuf)
{
	log_message(l_config, LOG_LEVEL_DEBUG, "disk_dev_statfs\n");
	int res;

	res = statvfs(path, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int disk_dev_release(const char *path, struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */
	log_message(l_config, LOG_LEVEL_DEBUG, "disk_dev_release\n");
	(void) path;
	(void) fi;
	return 0;
}

static int disk_dev_fsync(const char *path, int isdatasync,
		     struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */
	log_message(l_config, LOG_LEVEL_DEBUG, "disk_dev_fsync\n");
	(void) path;
	(void) isdatasync;
	(void) fi;
	return 0;
}

#ifdef HAVE_SETXATTR
/* xattr operations are optional and can safely be left unimplemented */
static int disk_dev_setxattr(const char *path, const char *name, const char *value,
			size_t size, int flags)
{
	int res = lsetxattr(path, name, value, size, flags);
	if (res == -1)
		return -errno;
	return 0;
}

static int disk_dev_getxattr(const char *path, const char *name, char *value,
			size_t size)
{
	int res = lgetxattr(path, name, value, size);
	if (res == -1)
		return -errno;
	return res;
}

static int disk_dev_listxattr(const char *path, char *list, size_t size)
{
	int res = llistxattr(path, list, size);
	if (res == -1)
		return -errno;
	return res;
}

static int disk_dev_removexattr(const char *path, const char *name)
{
	int res = lremovexattr(path, name);
	if (res == -1)
		return -errno;
	return 0;
}
#endif /* HAVE_SETXATTR */


/*
 * End of fuse operation
 */


static struct fuse_operations disk_dev_oper = {
	.getattr	= disk_dev_getattr,
	.access		= disk_dev_access,
	.readlink	= disk_dev_readlink,
	.readdir	= disk_dev_readdir,
	.mknod		= disk_dev_mknod,
	.mkdir		= disk_dev_mkdir,
	.symlink	= disk_dev_symlink,
	.unlink		= disk_dev_unlink,
	.rmdir		= disk_dev_rmdir,
	.rename		= disk_dev_rename,
	.link		= disk_dev_link,
	.chmod		= disk_dev_chmod,
	.chown		= disk_dev_chown,
	.truncate	= disk_dev_truncate,
	.utimens	= disk_dev_utimens,
	.open		= disk_dev_open,
	.read		= disk_dev_read,
	.write		= disk_dev_write,
	.statfs		= disk_dev_statfs,
	.release	= disk_dev_release,
	.fsync		= disk_dev_fsync,
#ifdef HAVE_SETXATTR
	.setxattr	= disk_dev_setxattr,
	.getxattr	= disk_dev_getxattr,
	.listxattr	= disk_dev_listxattr,
	.removexattr	= disk_dev_removexattr,
#endif
};

int DEFAULT_CC
fuse_process()
{
	struct fuse_args args = FUSE_ARGS_INIT(0, NULL);
	umask(0);

	log_message(l_config, LOG_LEVEL_DEBUG, "rdpdr_disk[main]: "
			"Configuration of fuse");
	fuse_opt_add_arg(&args, "");

	log_message(l_config, LOG_LEVEL_DEBUG, "rdpdr_disk[main]: "
			"Setup of the main mount point: %s", mount_point);
  fuse_opt_add_arg(&args, mount_point);

	return fuse_main(args.argc, args.argv, &disk_dev_oper, NULL);
}