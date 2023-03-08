//
// Created by sbreadf on 26.12.22.
//

#ifndef OS_2022_NETWORKFS_SBREAD_ENTRYPOINT_H
#define OS_2022_NETWORKFS_SBREAD_ENTRYPOINT_H

#endif  // OS_2022_NETWORKFS_SBREAD_ENTRYPOINT_H

#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/uaccess.h>

#include "http.h"

#define GET_TOKEN_FROM_INODE(inode) (const char *)((inode)->i_sb->s_fs_info)

struct content {
  u64 content_length;
  char content[512];
};

struct entry_info {
  unsigned char entry_type;  // DT_DIR (4) or DT_REG (8)
  ino_t ino;
};

struct entry {
  unsigned char entry_type;  // DT_DIR (4) or DT_REG (8)
  ino_t ino;
  char name[256];
};

struct entries {
  size_t entries_count;
  struct entry entries[16];
};

int networkfs_iterate(struct file *filp, struct dir_context *ctx);

struct dentry *networkfs_lookup(struct inode *parent_inode,
                                struct dentry *child_dentry, unsigned int flag);

int networkfs_create(struct user_namespace *, struct inode *parent_inode,
                     struct dentry *child_dentry, umode_t mode, bool b);

int networkfs_unlink(struct inode *parent_inode, struct dentry *child_dentry);

int networkfs_mkdir(struct user_namespace *u_nmspc, struct inode *parent_inode,
                    struct dentry *child_dentry, umode_t mode);

int networkfs_rmdir(struct inode *parent_inode, struct dentry *child_dentry);

ssize_t networkfs_read(struct file *filp, char *buffer, size_t len,
                       loff_t *offset);

ssize_t networkfs_write(struct file *filp, const char *buffer, size_t len,
                        loff_t *offset);

int networkfs_link(struct dentry *old_dentry, struct inode *parent_dir,
                   struct dentry *new_dentry);

void networkfs_kill_sb(struct super_block *sb);

struct inode *networkfs_get_inode(struct super_block *sb,
                                  const struct inode *dir, umode_t mode,
                                  int i_ino);

int networkfs_fill_super(struct super_block *sb, void *data, int silent);

struct dentry *networkfs_mount(struct file_system_type *fs_type, int flags,
                               const char *token, void *data);

char *name_convertor(char *name, size_t len);
